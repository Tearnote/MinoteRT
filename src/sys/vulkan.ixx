module;

#include <volk.h>
#include <GLFW/glfw3.h>
#include <VkBootstrap.h>

export module minote.vulkan;

import <utility>;
import <cassert>;
import <memory>;
import <vuk/Context.hpp>;
import "config.hpp";
import minote.service;
import minote.window;
import minote.except;
import minote.types;
import minote.math;
import minote.log;

// Handling of the elementary Vulkan objects
class Vulkan_impl:
	Log,
	Window
{
	// RAII wrappers for orderly construction and destruction
	struct RAIISurface {
		VkSurfaceKHR surface;
		vkb::Instance& instance;
		operator VkSurfaceKHR() const { return surface; }
		~RAIISurface() { vkDestroySurfaceKHR(instance, surface, nullptr); }
	};
	using RAIIInstance = std::unique_ptr<vkb::Instance, decltype([](auto* i) {
		vkb::destroy_instance(*i);
		delete i;
	})>;
	using RAIIDevice = std::unique_ptr<vkb::Device, decltype([](auto* d) {
		vkb::destroy_device(*d);
		delete d;
	})>;

public:

	RAIIInstance instance;
	RAIISurface surface;
	vkb::PhysicalDevice physicalDevice;
	RAIIDevice device;
	vuk::Context context;
	vuk::SwapchainRef swapchain;

	// Create a swapchain object, optionally reusing resources from an existing one.
	auto createSwapchain(uvec2 size, VkSwapchainKHR old = VK_NULL_HANDLE) -> vuk::Swapchain {
		auto vkbswapchainResult = vkb::SwapchainBuilder(*device)
			.set_old_swapchain(old)
			.set_desired_extent(size.x(), size.y())
			.set_desired_format({
				.format = VK_FORMAT_B8G8R8A8_UNORM,
				.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
			})
			.add_fallback_format({
				.format = VK_FORMAT_R8G8B8A8_UNORM,
				.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
			})
			.set_image_usage_flags(
				VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_DST_BIT |
				VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
			)
			.build();
		if (!vkbswapchainResult)
			throw runtime_error_fmt("Failed to create the swapchain: {}", vkbswapchainResult.error().message());
		auto vkbswapchain = vkbswapchainResult.value();

		auto vuksw = vuk::Swapchain{
			.swapchain = vkbswapchain.swapchain,
			.surface = surface,
			.format = vuk::Format(vkbswapchain.image_format),
			.extent = {vkbswapchain.extent.width, vkbswapchain.extent.height},
		};
		auto imgs = vkbswapchain.get_images();
		auto ivs = vkbswapchain.get_image_views();
		for (auto& img: *imgs)
			vuksw.images.emplace_back(vuk::Image{img, nullptr});
		for (auto& iv: *ivs)
			vuksw.image_views.emplace_back().payload = iv;
		return vuksw;
	}

	// Not movable, not copyable
	Vulkan_impl(Vulkan_impl const&) = delete;
	auto operator=(Vulkan_impl const&)->Vulkan_impl & = delete;

private:

	struct Queues {
		VkQueue graphics;
		uint graphicsFamilyIndex;
		VkQueue transfer;
		uint transferFamilyIndex;
		VkQueue compute;
		uint computeFamilyIndex;
	};

	// Can only be used as service
	friend struct Service<Vulkan_impl>;

	Vulkan_impl():
		instance(createInstance()),
		surface(createSurface(*instance)),
		physicalDevice(selectPhysicalDevice(*instance, surface)),
		device(createDevice(physicalDevice)),
		context(createContext(*instance, *device, physicalDevice, retrieveQueues(*device))),
		swapchain(context.add_swapchain(createSwapchain(Window::serv->size())))
	{
		info("Vulkan initialized");
	}

	~Vulkan_impl() { context.wait_idle(); }

#ifdef VK_VALIDATION
	static auto debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT severityCode,
		VkDebugUtilsMessageTypeFlagsEXT typeCode,
		VkDebugUtilsMessengerCallbackDataEXT const* data,
		void*
	) -> VkBool32 {
		assert(data);

		auto type = [typeCode]() {
			if (typeCode & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
				return "[VulkanPerf]";
			if (typeCode & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
				return "[VulkanSpec]";
			if (typeCode & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
				return "[Vulkan]";
			throw logic_error_fmt("Unknown Vulkan diagnostic message type: #{}", typeCode);
			}();

			class Logger: public Log {};
			auto logger = Logger();
			if (severityCode & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
				logger.error("{} {}", type, data->pMessage);
			else if (severityCode & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
				logger.warn("{} {}", type, data->pMessage);
			else if (severityCode & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
				logger.info("{} {}", type, data->pMessage);
			else if (severityCode & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
				logger.debug("{} {}", type, data->pMessage);
			else
				throw logic_error_fmt("Unknown Vulkan diagnostic message severity: #{}", std::to_underlying(severityCode));

			return VK_FALSE;
	}
#endif //VK_VALIDATION

	auto createInstance() -> RAIIInstance {
		auto instanceResult = vkb::InstanceBuilder()
#ifdef VK_VALIDATION
			.enable_layer("VK_LAYER_KHRONOS_validation")
			.add_validation_feature_enable(VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT)
			.add_validation_feature_enable(VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT)
			.set_debug_callback(debugCallback)
			.set_debug_messenger_severity(
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
				// Uncomment for debug printf
				// | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
			)
			.set_debug_messenger_type(
				VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
				VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT
			)
#endif //VK_VALIDATION
			.set_app_name(AppTitle)
			.set_engine_name("vuk")
			.require_api_version(1, 3, 0)
			.set_app_version(AppVersion[0], AppVersion[1], AppVersion[2])
			.build();
		if (!instanceResult)
			throw runtime_error_fmt("Failed to create a Vulkan instance: {}", instanceResult.error().message());
		auto instance = RAIIInstance(new vkb::Instance(instanceResult.value()));

		volkInitializeCustom(instance->fp_vkGetInstanceProcAddr);
		volkLoadInstanceOnly(instance->instance);

		debug("Vulkan instance created");
		return instance;
	}

	auto createSurface(vkb::Instance& instance) -> RAIISurface {
		auto result = RAIISurface{
			.surface = nullptr,
			.instance = instance,
		};
		glfwCreateWindowSurface(instance, Window::serv->handle(), nullptr, &result.surface);
		return result;
	}

	auto selectPhysicalDevice(vkb::Instance& instance, VkSurfaceKHR surface) -> vkb::PhysicalDevice {
		auto physicalDeviceFeatures = VkPhysicalDeviceFeatures{
#ifdef VK_VALIDATION
			.robustBufferAccess = VK_TRUE,
#endif //VK_VALIDATION
			.shaderStorageImageWriteWithoutFormat = VK_TRUE,
			.shaderInt64 = VK_TRUE,
		};
		auto physicalDeviceVulkan11Features = VkPhysicalDeviceVulkan11Features{
			.shaderDrawParameters = VK_TRUE,
		};
		auto physicalDeviceVulkan12Features = VkPhysicalDeviceVulkan12Features{
			.shaderSampledImageArrayNonUniformIndexing = VK_TRUE, // vuk requirement
			.descriptorBindingUpdateUnusedWhilePending = VK_TRUE, // vuk requirement
			.descriptorBindingPartiallyBound = VK_TRUE, // vuk requirement
			.descriptorBindingVariableDescriptorCount = VK_TRUE, // vuk requirement
			.runtimeDescriptorArray = VK_TRUE, // vuk requirement
			.hostQueryReset = VK_TRUE, // vuk requirement
			.timelineSemaphore = VK_TRUE, // vuk requirement
			.bufferDeviceAddress = VK_TRUE, // vuk requirement
			.vulkanMemoryModel = VK_TRUE, // general performance improvement
			.vulkanMemoryModelDeviceScope = VK_TRUE, // general performance improvement
			.shaderOutputLayer = VK_TRUE, // vuk requirement
		};
		auto physicalDeviceVulkan13Features = VkPhysicalDeviceVulkan13Features{
			.computeFullSubgroups = VK_TRUE,
			.synchronization2 = VK_TRUE, // pending vuk bugfix
			.maintenance4 = VK_TRUE,
		};

		auto physicalDeviceSelectorResult = vkb::PhysicalDeviceSelector(instance)
			.set_surface(surface)
			.set_minimum_version(1, 3)
			.set_required_features(physicalDeviceFeatures)
			.set_required_features_11(physicalDeviceVulkan11Features)
			.set_required_features_12(physicalDeviceVulkan12Features)
			.set_required_features_13(physicalDeviceVulkan13Features)
			.add_required_extension("VK_KHR_synchronization2")
#ifdef VK_VALIDATION
			.add_required_extension("VK_EXT_robustness2")
			.add_required_extension_features(VkPhysicalDeviceRobustness2FeaturesEXT{
				.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ROBUSTNESS_2_FEATURES_EXT,
				.robustBufferAccess2 = VK_TRUE,
				.robustImageAccess2 = VK_TRUE,
			})
#endif //VK_VALIDATION
			.prefer_gpu_device_type(vkb::PreferredDeviceType::discrete)
			.allow_any_gpu_device_type(false)
			.select(vkb::DeviceSelectionMode::partially_and_fully_suitable);
		if (!physicalDeviceSelectorResult)
			throw runtime_error_fmt("Failed to find a suitable GPU for Vulkan: {}",
				physicalDeviceSelectorResult.error().message()
			);
		auto physicalDevice = physicalDeviceSelectorResult.value();

		info("GPU selected: {}", physicalDevice.properties.deviceName);
		debug("Vulkan driver version {}.{}.{}",
			VK_API_VERSION_MAJOR(physicalDevice.properties.driverVersion),
			VK_API_VERSION_MINOR(physicalDevice.properties.driverVersion),
			VK_API_VERSION_PATCH(physicalDevice.properties.driverVersion));
		return physicalDevice;
	}

	auto createDevice(vkb::PhysicalDevice& physicalDevice) -> RAIIDevice {
		auto deviceResult = vkb::DeviceBuilder(physicalDevice).build();
		if (!deviceResult)
			throw runtime_error_fmt("Failed to create Vulkan device: {}", deviceResult.error().message());
		auto device = RAIIDevice(new vkb::Device(deviceResult.value()));
		volkLoadDevice(*device);

		debug("Vulkan device created");
		return device;
	}

	auto retrieveQueues(vkb::Device& device) -> Queues {
		auto result = Queues();

		result.graphics = device.get_queue(vkb::QueueType::graphics).value();
		result.graphicsFamilyIndex = device.get_queue_index(vkb::QueueType::graphics).value();

		auto transferQueuePresent = device.get_dedicated_queue(vkb::QueueType::transfer).has_value();
		result.transfer = transferQueuePresent?
			device.get_dedicated_queue(vkb::QueueType::transfer).value() :
			VK_NULL_HANDLE;
		result.transferFamilyIndex = transferQueuePresent?
			device.get_dedicated_queue_index(vkb::QueueType::transfer).value() :
			VK_QUEUE_FAMILY_IGNORED;

		auto computeQueuePresent = device.get_dedicated_queue(vkb::QueueType::compute).has_value();
		result.compute = computeQueuePresent?
			device.get_dedicated_queue(vkb::QueueType::compute).value() :
			VK_NULL_HANDLE;
		result.computeFamilyIndex = computeQueuePresent?
			device.get_dedicated_queue_index(vkb::QueueType::compute).value() :
			VK_QUEUE_FAMILY_IGNORED;

		return result;
	}

	auto createContext(vkb::Instance& instance, vkb::Device& device,
		vkb::PhysicalDevice& physicalDevice, Queues const& queues) -> vuk::Context
	{
		return vuk::Context(vuk::ContextCreateParameters{
			.instance = instance,
			.device = device,
			.physical_device = physicalDevice.physical_device,
			.graphics_queue = queues.graphics,
			.graphics_queue_family_index = queues.graphicsFamilyIndex,
			.compute_queue = queues.compute,
			.compute_queue_family_index = queues.computeFamilyIndex,
			.transfer_queue = queues.transfer,
			.transfer_queue_family_index = queues.transferFamilyIndex,
			.pointers = vuk::ContextCreateParameters::FunctionPointers{
				// Kill me now
				.vkGetInstanceProcAddr = vkGetInstanceProcAddr,
				.vkGetDeviceProcAddr = vkGetDeviceProcAddr,
				.vkCmdBindDescriptorSets = vkCmdBindDescriptorSets,
				.vkCmdBindIndexBuffer = vkCmdBindIndexBuffer,
				.vkCmdBindPipeline = vkCmdBindPipeline,
				.vkCmdBindVertexBuffers = vkCmdBindVertexBuffers,
				.vkCmdBlitImage = vkCmdBlitImage,
				.vkCmdClearColorImage = vkCmdClearColorImage,
				.vkCmdClearDepthStencilImage = vkCmdClearDepthStencilImage,
				.vkCmdCopyBuffer = vkCmdCopyBuffer,
				.vkCmdCopyBufferToImage = vkCmdCopyBufferToImage,
				.vkCmdCopyImageToBuffer = vkCmdCopyImageToBuffer,
				.vkCmdFillBuffer = vkCmdFillBuffer,
				.vkCmdUpdateBuffer = vkCmdUpdateBuffer,
				.vkCmdResolveImage = vkCmdResolveImage,
				.vkCmdPipelineBarrier = vkCmdPipelineBarrier,
				.vkCmdWriteTimestamp = vkCmdWriteTimestamp,
				.vkCmdDraw = vkCmdDraw,
				.vkCmdDrawIndexed = vkCmdDrawIndexed,
				.vkCmdDrawIndexedIndirect = vkCmdDrawIndexedIndirect,
				.vkCmdDispatch = vkCmdDispatch,
				.vkCmdDispatchIndirect = vkCmdDispatchIndirect,
				.vkCmdPushConstants = vkCmdPushConstants,
				.vkCmdSetViewport = vkCmdSetViewport,
				.vkCmdSetScissor = vkCmdSetScissor,
				.vkCmdSetLineWidth = vkCmdSetLineWidth,
				.vkCmdSetDepthBias = vkCmdSetDepthBias,
				.vkCmdSetBlendConstants = vkCmdSetBlendConstants,
				.vkCmdSetDepthBounds = vkCmdSetDepthBounds,
				.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties,
				.vkCreateFramebuffer = vkCreateFramebuffer,
				.vkDestroyFramebuffer = vkDestroyFramebuffer,
				.vkCreateCommandPool = vkCreateCommandPool,
				.vkResetCommandPool = vkResetCommandPool,
				.vkDestroyCommandPool = vkDestroyCommandPool,
				.vkAllocateCommandBuffers = vkAllocateCommandBuffers,
				.vkBeginCommandBuffer = vkBeginCommandBuffer,
				.vkEndCommandBuffer = vkEndCommandBuffer,
				.vkFreeCommandBuffers = vkFreeCommandBuffers,
				.vkCreateDescriptorPool = vkCreateDescriptorPool,
				.vkResetDescriptorPool = vkResetDescriptorPool,
				.vkDestroyDescriptorPool = vkDestroyDescriptorPool,
				.vkAllocateDescriptorSets = vkAllocateDescriptorSets,
				.vkUpdateDescriptorSets = vkUpdateDescriptorSets,
				.vkCreateGraphicsPipelines = vkCreateGraphicsPipelines,
				.vkCreateComputePipelines = vkCreateComputePipelines,
				.vkDestroyPipeline = vkDestroyPipeline,
				.vkCreateQueryPool = vkCreateQueryPool,
				.vkGetQueryPoolResults = vkGetQueryPoolResults,
				.vkDestroyQueryPool = vkDestroyQueryPool,
				.vkCreatePipelineCache = vkCreatePipelineCache,
				.vkGetPipelineCacheData = vkGetPipelineCacheData,
				.vkDestroyPipelineCache = vkDestroyPipelineCache,
				.vkCreateRenderPass = vkCreateRenderPass,
				.vkCmdBeginRenderPass = vkCmdBeginRenderPass,
				.vkCmdNextSubpass = vkCmdNextSubpass,
				.vkCmdEndRenderPass = vkCmdEndRenderPass,
				.vkDestroyRenderPass = vkDestroyRenderPass,
				.vkCreateSampler = vkCreateSampler,
				.vkDestroySampler = vkDestroySampler,
				.vkCreateShaderModule = vkCreateShaderModule,
				.vkDestroyShaderModule = vkDestroyShaderModule,
				.vkCreateImageView = vkCreateImageView,
				.vkDestroyImageView = vkDestroyImageView,
				.vkCreateDescriptorSetLayout = vkCreateDescriptorSetLayout,
				.vkDestroyDescriptorSetLayout = vkDestroyDescriptorSetLayout,
				.vkCreatePipelineLayout = vkCreatePipelineLayout,
				.vkDestroyPipelineLayout = vkDestroyPipelineLayout,
				.vkCreateFence = vkCreateFence,
				.vkWaitForFences = vkWaitForFences,
				.vkDestroyFence = vkDestroyFence,
				.vkCreateSemaphore = vkCreateSemaphore,
				.vkWaitSemaphores = vkWaitSemaphores,
				.vkDestroySemaphore = vkDestroySemaphore,
				.vkQueueSubmit = vkQueueSubmit,
				.vkDeviceWaitIdle = vkDeviceWaitIdle,
				.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties,
				.vkAllocateMemory = vkAllocateMemory,
				.vkFreeMemory = vkFreeMemory,
				.vkMapMemory = vkMapMemory,
				.vkUnmapMemory = vkUnmapMemory,
				.vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges,
				.vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges,
				.vkBindBufferMemory = vkBindBufferMemory,
				.vkBindImageMemory = vkBindImageMemory,
				.vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements,
				.vkGetImageMemoryRequirements = vkGetImageMemoryRequirements,
				.vkCreateBuffer = vkCreateBuffer,
				.vkDestroyBuffer = vkDestroyBuffer,
				.vkCreateImage = vkCreateImage,
				.vkDestroyImage = vkDestroyImage,
				.vkGetPhysicalDeviceProperties2 = vkGetPhysicalDeviceProperties2,
				.vkGetBufferDeviceAddress = vkGetBufferDeviceAddress,
				.vkCmdDrawIndexedIndirectCount = vkCmdDrawIndexedIndirectCount,
				.vkResetQueryPool = vkResetQueryPool,
				.vkAcquireNextImageKHR = vkAcquireNextImageKHR,
				.vkQueuePresentKHR = vkQueuePresentKHR,
				.vkDestroySwapchainKHR = vkDestroySwapchainKHR,
				.vkSetDebugUtilsObjectNameEXT = vkSetDebugUtilsObjectNameEXT,
				.vkCmdBeginDebugUtilsLabelEXT = vkCmdBeginDebugUtilsLabelEXT,
				.vkCmdEndDebugUtilsLabelEXT = vkCmdEndDebugUtilsLabelEXT,
				.vkCmdBuildAccelerationStructuresKHR = vkCmdBuildAccelerationStructuresKHR,
				.vkGetAccelerationStructureBuildSizesKHR = vkGetAccelerationStructureBuildSizesKHR,
				.vkCmdTraceRaysKHR = vkCmdTraceRaysKHR,
				.vkCreateAccelerationStructureKHR = vkCreateAccelerationStructureKHR,
				.vkDestroyAccelerationStructureKHR = vkDestroyAccelerationStructureKHR,
				.vkGetRayTracingShaderGroupHandlesKHR = vkGetRayTracingShaderGroupHandlesKHR,
				.vkCreateRayTracingPipelinesKHR = vkCreateRayTracingPipelinesKHR,
			},
		});
	}
};

export using Vulkan = Service<Vulkan_impl>;
