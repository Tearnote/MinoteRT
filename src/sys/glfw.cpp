#include "sys/glfw.hpp"

#include <stdexcept>
#include <string>
#ifndef NOMINMAX
#define NOMINMAX
#endif //NOMINMAX
// WIN32_LEAN_AND_MEAN would remove parts we actually need here
#include <windows.h>
#include <timeapi.h>
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

Glfw::Glfw(std::string_view _title, uvec2 _size) {

	ASSUME(_size.x() > 0 && _size.y() > 0);

	// Increase sleep timer resolution
	if (timeBeginPeriod(1) != TIMERR_NOERROR)
		throw std::runtime_error("Failed to initialize Windows timer");

	// Convert any glfw error into an exception on the calling thread
	glfwSetErrorCallback([](int code, char const* str) {
		throw stx::runtime_error_fmt("GLFW error {}: {}", code, str);
	});
	glfwInit();

	L_DEBUG("GLFW initialized");

	// Create the window
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	m_window = glfwCreateWindow(_size.x(), _size.y(), std::string(_title).c_str(), nullptr, nullptr);
	ASSUME(m_window);

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

}

auto Glfw::isClosing() -> bool {

	return glfwWindowShouldClose(m_window);

}

}
