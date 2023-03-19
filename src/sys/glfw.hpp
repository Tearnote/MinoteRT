#pragma once

#include <string_view>
#include <functional>

#include "math.hpp"
#include "stx/time.hpp"
#include "util/service.hpp"

// Forward declarations
struct GLFWwindow;

namespace minote::sys {

// OS-specific functionality - windowing, event queue etc.
class Glfw {

public:

	// Create a console window and bind to standard input and output
	static void initConsole();

	// Set current thread's name to the provided string. Does nothing if THREAD_DEBUG is not set
	static void setThreadName(std::string_view);

	// Call this as often as possible to process system and input events
	void poll();

	// Returns true if window close is requested and the application should quit
	auto isClosing() -> bool;

	// Return the raw window handle, useful for Vulkan surface creation
	auto windowHandle() -> GLFWwindow* { return m_window; }

	auto windowSize() -> uvec2;

	auto getCursorPosition() -> vec2;

	void registerKeyCallback(std::function<void(int, bool)>);
	void registerCursorMotionCallback(std::function<void(vec2)>);
	void registerMouseButtonCallback(std::function<void(int, bool)>);

	// Not movable, not copyable
	Glfw(Glfw const&) = delete;
	auto operator=(Glfw const&) -> Glfw& = delete;

private:

	GLFWwindow* m_window;

	std::vector<std::function<void(int, bool)>> m_keyCallbacks;
	std::vector<std::function<void(vec2)>> m_cursorMotionCallbacks;
	std::vector<std::function<void(int, bool)>> m_mouseButtonCallbacks;

	// Only usable from the service
	friend struct util::Service<Glfw>;

	// Initialize GLFW and open a window with specified parameters on the screen
	explicit Glfw(std::string_view title, uvec2 size = {1280, 720});
	~Glfw();

};

inline util::Service<Glfw> s_glfw;

}
