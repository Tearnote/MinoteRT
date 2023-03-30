#include "sys/glfw.hpp"

#include "config.hpp"

#include <stdexcept>
#include <string>
#ifndef NOMINMAX
#define NOMINMAX
#endif //NOMINMAX
// WIN32_LEAN_AND_MEAN would remove parts we actually need here
#include <windows.h>
#include <timeapi.h>
#include <processthreadsapi.h>
#include <fcntl.h>
#include <stdio.h>
#include <io.h>

#include <volk.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "math.hpp"
#include "log.hpp"
#include "stx/verify.hpp"
#include "stx/except.hpp"
#include "stx/time.hpp"

namespace minote::sys {

using namespace stx::time_literals;

void Glfw::setThreadName(std::string_view _name) {

#ifdef THREAD_DEBUG
	auto lname = std::wstring(_name.begin(), _name.end());
	SetThreadDescription(GetCurrentThread(), lname.c_str());
#endif //THREAD_DEBUG

}

Glfw::Glfw(std::string_view _title, uvec2 _size) {

	ASSUME(_size.x() > 0 && _size.y() > 0);

	// Increase sleep timer resolution
	if (timeBeginPeriod(1) != TIMERR_NOERROR)
		throw std::runtime_error("Failed to initialize Windows timer");

	// Convert any glfw error into an exception on the calling thread
	glfwSetErrorCallback([](int code, char const* str) {
		throw stx::runtime_error_fmt("[GLFW] Error {}: {}", code, str);
	});
	glfwInit();

	L_DEBUG("GLFW initialized");

	// Create the window
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	m_window = glfwCreateWindow(int(_size.x()), int(_size.y()), std::string(_title).c_str(), nullptr, nullptr);
	ASSUME(m_window);

	// Set up event callbacks
	glfwSetWindowUserPointer(m_window, this);

	glfwSetKeyCallback(m_window, [](GLFWwindow* window, int key, int, int action, int) {
		if (action == GLFW_REPEAT) return;
		auto& glfw = *reinterpret_cast<Glfw*>(glfwGetWindowUserPointer(window));
		for (auto& func: glfw.m_keyCallbacks)
			func(key, action == GLFW_PRESS);
	});

	glfwSetCursorPosCallback(m_window, [](GLFWwindow* window, double x, double y) {
		auto& glfw = *reinterpret_cast<Glfw*>(glfwGetWindowUserPointer(window));
		for (auto& func: glfw.m_cursorMotionCallbacks)
			func(vec2{float(x), float(y)});
	});

	glfwSetMouseButtonCallback(m_window, [](GLFWwindow* window, int button, int action, int) {
		auto& glfw = *reinterpret_cast<Glfw*>(glfwGetWindowUserPointer(window));
		for (auto& func: glfw.m_mouseButtonCallbacks)
			func(button, action == GLFW_PRESS);
	});

	// Quit on ESC
	registerKeyCallback([this](int key, bool) {
		if (key == GLFW_KEY_ESCAPE)
			glfwSetWindowShouldClose(m_window, GLFW_TRUE);
	});

	L_INFO("Window {} created at {}x{}", _title, _size.x(), _size.y());

}

Glfw::~Glfw() {

	glfwDestroyWindow(m_window);
	glfwTerminate();
	timeEndPeriod(1);
	L_DEBUG("GLFW cleaned up");

}

void Glfw::initConsole() {

	// https://github.com/ocaml/ocaml/issues/9252#issuecomment-576383814
	AllocConsole();

	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);

	int fdOut = _open_osfhandle(reinterpret_cast<intptr_t>(GetStdHandle(STD_OUTPUT_HANDLE)), _O_WRONLY | _O_BINARY);
	int fdErr = _open_osfhandle(reinterpret_cast<intptr_t>(GetStdHandle(STD_ERROR_HANDLE)),  _O_WRONLY | _O_BINARY);

	if (fdOut) {
		_dup2(fdOut, 1);
		_close(fdOut);
		SetStdHandle(STD_OUTPUT_HANDLE, reinterpret_cast<HANDLE>(_get_osfhandle(1)));
	}
	if (fdErr) {
		_dup2(fdErr, 2);
		_close(fdErr);
		SetStdHandle(STD_ERROR_HANDLE, reinterpret_cast<HANDLE>(_get_osfhandle(2)));
	}

	_dup2(_fileno(fdopen(1, "wb")), _fileno(stdout));
	_dup2(_fileno(fdopen(2, "wb")), _fileno(stderr));

	setvbuf(stdout, nullptr, _IONBF, 0);
	setvbuf(stderr, nullptr, _IONBF, 0);

	// Set console encoding to UTF-8
	SetConsoleOutputCP(65001);

	// Enable ANSI color code support
	auto out = GetStdHandle(STD_OUTPUT_HANDLE);
	auto mode = 0ul;
	GetConsoleMode(out, &mode);
	mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	SetConsoleMode(out, mode);

}

void Glfw::poll() {

	glfwPollEvents();

}

auto Glfw::isClosing() -> bool {

	return glfwWindowShouldClose(m_window);

}

auto Glfw::windowSize() -> uvec2 {

	auto w = 0;
	auto h = 0;
	glfwGetFramebufferSize(m_window, &w, &h);
	return uvec2{uint(w), uint(h)};

}

auto Glfw::getCursorPosition() -> vec2 {

	double x;
	double y;
	glfwGetCursorPos(m_window, &x, &y);
	return vec2{float(x), float(y)};

}

void Glfw::registerKeyCallback(std::function<void(int, bool)> _func) {

	m_keyCallbacks.emplace_back(std::move(_func));

}

void Glfw::registerCursorMotionCallback(std::function<void(vec2)> _func) {

	m_cursorMotionCallbacks.emplace_back(std::move(_func));

}

void Glfw::registerMouseButtonCallback(std::function<void(int, bool)> _func) {

	m_mouseButtonCallbacks.emplace_back(std::move(_func));

}

auto Glfw::getTime() -> stx::nsec {

	return stx::seconds(glfwGetTime());

}

}
