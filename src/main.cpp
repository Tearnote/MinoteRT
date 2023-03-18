#include "config.hpp"

#include <exception>
#include <iostream>
#include <cstdlib>

#include "log.hpp"
#include "sys/vulkan.hpp"
#include "sys/glfw.hpp"
#include "gfx/renderer.hpp"

auto WinMain(HINSTANCE, HINSTANCE, LPSTR, int) -> int try {

	using namespace minote; // main itself cannot be namespaced

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

	// Main loop
	while(!sys::s_glfw->isClosing()) {

		// Handle user and system events
		sys::s_glfw->poll();

		// Draw the next frame
		renderer.draw();

	}

	// Clean shutdown
	return EXIT_SUCCESS;

} catch (std::exception& e) {
	std::cout << "Uncaught exception on main thread: " << e.what() << "\n";
	return EXIT_FAILURE;
}
