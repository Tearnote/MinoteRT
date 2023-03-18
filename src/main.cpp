#include "config.hpp"

#include <exception>
#include <iostream>
#include <cstdlib>
#include <thread>
#include <chrono>

#include "log.hpp"
#include "sys/vulkan.hpp"
#include "sys/glfw.hpp"

auto WinMain(HINSTANCE, HINSTANCE, LPSTR, int) -> int try {

	using namespace minote; // main itself cannot be namespaced

	sys::Glfw::setThreadName("main");
	sys::Glfw::initConsole();
	Log::init(Log_p, LOG_LEVEL);
	L_INFO("Starting up {} {}.{}.{}", AppTitle,
		   AppVersion[0], AppVersion[1], AppVersion[2]);

	auto glfw = sys::s_glfw.provide("MinoteRT");
	auto vulkan = sys::s_vulkan.provide();

	while(!sys::s_glfw->isClosing()) {
		sys::s_glfw->poll();
		std::this_thread::sleep_for(std::chrono::milliseconds(1)); // Relinquish timeslice
	}
	return EXIT_SUCCESS;

} catch (std::exception& e) {
	std::cout << "Uncaught exception on main thread: " << e.what() << "\n";
	return EXIT_FAILURE;
}
