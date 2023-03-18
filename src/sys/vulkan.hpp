#pragma once

#include <optional>

#include <volk.h> // Order is important
#include <VkBootstrap.h>
#include <vuk/Context.hpp>

#include "types.hpp"
#include "math.hpp"
#include "log.hpp"
#include "util/service.hpp"

namespace minote::sys {

// Handling of the elementary Vulkan objects
class Vulkan {

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
	auto createSwapchain(uvec2 size, VkSwapchainKHR old = VK_NULL_HANDLE) -> vuk::Swapchain;

	// Not movable, not copyable
	Vulkan(Vulkan const&) = delete;
	auto operator=(Vulkan const&) -> Vulkan& = delete;

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
	friend struct util::Service<Vulkan>;
	Vulkan();
	~Vulkan();

	static auto createInstance() -> RAIIInstance;
	static auto createSurface(vkb::Instance&) -> RAIISurface;
	static auto selectPhysicalDevice(vkb::Instance&, VkSurfaceKHR) -> vkb::PhysicalDevice;
	static auto createDevice(vkb::PhysicalDevice&) -> RAIIDevice;
	static auto retrieveQueues(vkb::Device&) -> Queues;
	static auto createContext(vkb::Instance&, vkb::Device&, vkb::PhysicalDevice&, Queues const&) -> vuk::Context;

};

inline util::Service<Vulkan> s_vulkan;

}
