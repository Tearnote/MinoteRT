#pragma once

#include "math.hpp"
#include "gfx/camera.hpp"

namespace minote {

// A camera controller for free flying movement anywhere in the world
class Freecam {

public:

	Freecam();
	// Register callbacks with GLFW
	void registerEvents();
	// Apply freecam to a camera
	void updateCamera(gfx::Camera&);

private:

	bool up = false;
	bool down = false;
	bool left = false;
	bool right = false;
	bool floating = false;
	bool moving = false;
	vec2 offset = {0.0f, 0.0f};

	vec2 prevCursorPos = {0.0f, 0.0f};

};

}
