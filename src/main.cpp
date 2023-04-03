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
	auto renderer = gfx::Renderer();
	auto camera = gfx::Camera{
		.position = {0.0f, -1.0f, 0.0f},
		.yaw = 270_deg,
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
		renderer.draw(camera);

	}

	// Clean shutdown
	return EXIT_SUCCESS;

} catch (std::exception& e) {
	std::cout << "Uncaught exception on main thread: " << e.what() << "\n";
	return EXIT_FAILURE;
}
