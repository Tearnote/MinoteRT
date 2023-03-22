#include "sys/vulkan.hpp"

#include "config.hpp"

#include <volk.h> // Order is important
#include <GLFW/glfw3.h>

#include "log.hpp"
#include "stx/verify.hpp"
#include "stx/except.hpp"
#include "sys/glfw.hpp"

namespace minote::sys {

#ifdef VK_VALIDATION
VKAPI_ATTR auto VKAPI_CALL debugCallback(
	VkDebugUtilsMessageSeverityFlagBitsEXT _severityCode,
	VkDebugUtilsMessageTypeFlagsEXT _typeCode,
	VkDebugUtilsMessengerCallbackDataEXT const* _data,
	void*) -> VkBool32 {

	ASSUME(_data);

	auto type = [_typeCode]() {
		if (_typeCode & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
			return "[VulkanPerf]";
		if (_typeCode & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT)
			return "[VulkanSpec]";
		if (_typeCode & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
			return "[Vulkan]";
		throw stx::logic_error_fmt("Unknown Vulkan diagnostic message type: #{}", _typeCode);
	}();

	if (_severityCode & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
		L_ERROR("{} {}", type, _data->pMessage);
	else if (_severityCode & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		L_WARN("{} {}", type, _data->pMessage);
	else if (_severityCode & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
		L_INFO("{} {}", type, _data->pMessage);
	else if (_severityCode & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
		L_DEBUG("{} {}", type, _data->pMessage);
	else
		throw stx::logic_error_fmt("Unknown Vulkan diagnostic message severity: #{}", +_severityCode);

	return VK_FALSE;

}
#endif

Vulkan::Vulkan():
	instance(createInstance()),
	surface(createSurface(*instance)),
	physicalDevice(selectPhysicalDevice(*instance, surface)),
	device(createDevice(physicalDevice)),
	context(createContext(*instance, *device, physicalDevice, retrieveQueues(*device))),
	swapchain(context.add_swapchain(createSwapchain(sys::s_glfw->windowSize()))) {

	L_INFO("Vulkan initialized");

}

Vulkan::~Vulkan() {

	context.wait_idle();

}

auto Vulkan::createSwapchain(uvec2 _size, VkSwapchainKHR _old) -> vuk::Swapchain {

	auto vkbswapchainResult = vkb::SwapchainBuilder(*device)
		.set_old_swapchain(_old)
		.set_desired_extent(_size.x(), _size.y())
		.set_desired_format({
			.format = VK_FORMAT_B8G8R8A8_UNORM,
			.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
		})
		.add_fallback_format({
			.format = VK_FORMAT_R8G8B8A8_UNORM,
			.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
		})
		.set_image_usage_flags(VkImageUsageFlagBits::VK_IMAGE_USAGE_TRANSFER_DST_BIT |
		                       VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
		.build();
	if (!vkbswapchainResult)
		throw stx::runtime_error_fmt("Failed to create the swapchain: {}", vkbswapchainResult.error().message());
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

auto Vulkan::createInstance() -> RAIIInstance {

	auto instanceResult = vkb::InstanceBuilder()
#ifdef VK_VALIDATION
		.enable_layer("VK_LAYER_KHRONOS_validation")
		//.add_validation_feature_enable(VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT) // Disabled due to false positives (yes, definitely)
		.add_validation_feature_enable(VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT)
		.set_debug_callback(debugCallback)
		.set_debug_messenger_severity(
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT/* |
			VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT*/) // Uncomment for debug printf
		.set_debug_messenger_type(
			VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT)
#endif
		.set_app_name(AppTitle)
		.set_engine_name("vuk")
		.require_api_version(1, 3, 0)
		.set_app_version(AppVersion[0], AppVersion[1], AppVersion[2])
		.build();
	if (!instanceResult)
		throw stx::runtime_error_fmt("Failed to create a Vulkan instance: {}", instanceResult.error().message());
	auto instance = RAIIInstance(new vkb::Instance(instanceResult.value()));

	volkInitializeCustom(instance->fp_vkGetInstanceProcAddr);
	volkLoadInstanceOnly(instance->instance);

	L_DEBUG("Vulkan instance created");
	return instance;

}

auto Vulkan::createSurface(vkb::Instance& _instance) -> RAIISurface {

	auto result = RAIISurface{
		.surface = nullptr,
		.instance = _instance,
	};
	glfwCreateWindowSurface(_instance, s_glfw->windowHandle(), nullptr, &result.surface);
	return result;

}

auto Vulkan::selectPhysicalDevice(vkb::Instance& _instance, VkSurfaceKHR _surface) -> vkb::PhysicalDevice {

	auto physicalDeviceFeatures = VkPhysicalDeviceFeatures{
#ifdef VK_VALIDATION
		.robustBufferAccess = VK_TRUE,
#endif
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

	auto physicalDeviceSelectorResult = vkb::PhysicalDeviceSelector(_instance)
		.set_surface(_surface)
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
			.robustImageAccess2 = VK_TRUE
		})
#endif
		.prefer_gpu_device_type(vkb::PreferredDeviceType::discrete)
		.allow_any_gpu_device_type(false)
		.select(vkb::DeviceSelectionMode::partially_and_fully_suitable);
	if (!physicalDeviceSelectorResult)
		throw stx::runtime_error_fmt("Failed to find a suitable GPU for Vulkan: {}",
			physicalDeviceSelectorResult.error().message());
	auto physicalDevice = physicalDeviceSelectorResult.value();

	L_INFO("GPU selected: {}", physicalDevice.properties.deviceName);
	L_DEBUG("Vulkan driver version {}.{}.{}",
		VK_API_VERSION_MAJOR(physicalDevice.properties.driverVersion),
		VK_API_VERSION_MINOR(physicalDevice.properties.driverVersion),
		VK_API_VERSION_PATCH(physicalDevice.properties.driverVersion));
	return physicalDevice;

}

auto Vulkan::createDevice(vkb::PhysicalDevice& _physicalDevice) -> RAIIDevice {

	auto deviceResult = vkb::DeviceBuilder(_physicalDevice).build();
	if (!deviceResult)
		throw stx::runtime_error_fmt("Failed to create Vulkan device: {}", deviceResult.error().message());
	auto device = RAIIDevice(new vkb::Device(deviceResult.value()));

	volkLoadDevice(*device);

	L_DEBUG("Vulkan device created");
	return device;

}

auto Vulkan::retrieveQueues(vkb::Device& _device) -> Queues {

	auto result = Queues();

	result.graphics = _device.get_queue(vkb::QueueType::graphics).value();
	result.graphicsFamilyIndex = _device.get_queue_index(vkb::QueueType::graphics).value();

	auto transferQueuePresent = _device.get_dedicated_queue(vkb::QueueType::transfer).has_value();
	result.transfer = transferQueuePresent?
		_device.get_dedicated_queue(vkb::QueueType::transfer).value() :
		VK_NULL_HANDLE;
	result.transferFamilyIndex = transferQueuePresent?
		_device.get_dedicated_queue_index(vkb::QueueType::transfer).value() :
		VK_QUEUE_FAMILY_IGNORED;

	auto computeQueuePresent = _device.get_dedicated_queue(vkb::QueueType::compute).has_value();
	result.compute = computeQueuePresent?
		_device.get_dedicated_queue(vkb::QueueType::compute).value() :
		VK_NULL_HANDLE;
	result.computeFamilyIndex = computeQueuePresent?
		_device.get_dedicated_queue_index(vkb::QueueType::compute).value() :
		VK_QUEUE_FAMILY_IGNORED;

	return result;

}

auto Vulkan::createContext(vkb::Instance& _instance, vkb::Device& _device,
	vkb::PhysicalDevice& _physicalDevice, Queues const& _queues) -> vuk::Context {

	return vuk::Context(vuk::ContextCreateParameters{
		.instance =                    _instance,
		.device =                      _device,
		.physical_device =             _physicalDevice.physical_device,
		.graphics_queue =              _queues.graphics,
		.graphics_queue_family_index = _queues.graphicsFamilyIndex,
		.compute_queue =               _queues.compute,
		.compute_queue_family_index =  _queues.computeFamilyIndex,
		.transfer_queue =              _queues.transfer,
		.transfer_queue_family_index = _queues.transferFamilyIndex,
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

}
