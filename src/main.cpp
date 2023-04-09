#include "config.hpp"

#include <exception>
#include <iostream>
#include <cstdlib>

#include "freecam.hpp"
#include "log.hpp"
#include "sys/vulkan.hpp"
#include "sys/glfw.hpp"
#include "gfx/renderer.hpp"
#include "gfx/camera.hpp"

auto WinMain(HINSTANCE, HINSTANCE, LPSTR, int) -> int try {

	using namespace minote; // main itself cannot be namespaced
    using namespace math_literals;

	// Initializing basic output
	sys::Glfw::setThreadName("main");
	sys::Glfw::initConsole();
	Log::init(Log_p, LOG_LEVEL);
	L_INFO("Starting up {} {}.{}.{}", AppTitle,
		   AppVersion[0], AppVersion[1], AppVersion[2]);

	// Initializing subsystems
	auto glfw = sys::s_glfw.provide("MinoteRT");
	auto vulkan = sys::s_vulkan.provide();
	auto renderer = gfx::s_renderer.provide();
	auto camera = gfx::Camera{
		.viewport = {
			sys::s_vulkan->swapchain->extent.width,
			sys::s_vulkan->swapchain->extent.height,
		},
		.verticalFov = 60_deg,
		.nearPlane = 0.001f,
		.position = {0.0f, -0.001f, 0.1f},
		.yaw = 90_deg,
		.pitch = 0.0f,
		.lookSpeed = 1.0f / 256.0f,
		.moveSpeed = 8.0f,
	};
	auto freecam = Freecam();
	freecam.registerEvents();

	// Main loop
	while(!sys::s_glfw->isClosing()) {

		// Handle user and system events
		sys::s_glfw->poll();

		freecam.updateCamera(camera);

		// Draw the next frame
		gfx::s_renderer->draw(camera);

	}

	// Clean shutdown
	return EXIT_SUCCESS;

} catch (std::exception& e) {
	std::cout << "Uncaught exception on main thread: " << e.what() << "\n";
	return EXIT_FAILURE;
}
