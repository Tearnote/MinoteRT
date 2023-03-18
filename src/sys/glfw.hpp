#pragma once

#include <string_view>

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

	// Returns true if window close is requested and the application should quit
	auto isClosing() -> bool;

	// Not movable, not copyable
	Glfw(Glfw const&) = delete;
	auto operator=(Glfw const&) -> Glfw& = delete;

private:

	GLFWwindow* m_window;

	// Only usable from the service
	friend struct util::Service<Glfw>;

	// Initialize GLFW and open a window with specified parameters on the screen
	explicit Glfw(std::string_view title, uvec2 size = {1280, 720});
	~Glfw();

};

inline util::Service<Glfw> s_glfw;

}
