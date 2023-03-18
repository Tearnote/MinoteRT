#include "config.hpp"

#include <exception>
#include <iostream>
#include <cstdlib>
#include <thread>
#include <chrono>

#ifdef THREAD_DEBUG
#ifndef NOMINMAX
#define NOMINMAX
#endif //NOMINMAX
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif //WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <processthreadsapi.h>
#endif //THREAD_DEBUG

#include "log.hpp"
#include "sys/glfw.hpp"

auto WinMain(HINSTANCE, HINSTANCE, LPSTR, int) -> int try {

	using namespace minote; // main itself cannot be namespaced

#ifdef THREAD_DEBUG
	SetThreadDescription(GetCurrentThread(), L"main");
#endif

	sys::Glfw::initConsole();
	Log::init(Log_p, LOG_LEVEL);
	L_INFO("Starting up {} {}.{}.{}", AppTitle,
		   AppVersion[0], AppVersion[1], AppVersion[2]);

	auto glfw = sys::s_glfw.provide("MinoteRT");

	while(!sys::s_glfw->isClosing())
		std::this_thread::sleep_for(std::chrono::milliseconds(1)); // Relinquish timeslice

	return EXIT_SUCCESS;

} catch (std::exception& e) {
	std::cout << "Uncaught exception on main thread: " << e.what() << "\n";
	return EXIT_FAILURE;
}
