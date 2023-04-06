#include "freecam.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <imgui.h>

#include "sys/glfw.hpp"
#include "gfx/renderer.hpp"

namespace minote {

using namespace math_literals;

Freecam::Freecam() {

	prevCursorPos = sys::s_glfw->getCursorPosition();

}

void Freecam::registerEvents() {
	sys::s_glfw->registerKeyCallback([this](int key, bool action) {
		if (action && ImGui::GetIO().WantCaptureKeyboard) return;
		switch (key) {
			case GLFW_KEY_UP:
			case GLFW_KEY_W:
				up = action; return;
			case GLFW_KEY_DOWN:
			case GLFW_KEY_S:
				down = action; return;
			case GLFW_KEY_LEFT:
			case GLFW_KEY_A:
				left = action; return;
			case GLFW_KEY_RIGHT:
			case GLFW_KEY_D:
				right = action; return;
			case GLFW_KEY_SPACE:
				floating = action; return;
		}
	});
	sys::s_glfw->registerCursorMotionCallback([this](vec2 newPos) {
		offset += newPos - prevCursorPos;
		prevCursorPos = newPos;
	});
	sys::s_glfw->registerMouseButtonCallback([this](int button, bool action) {
		if (action && ImGui::GetIO().WantCaptureMouse) return;
		if (button == GLFW_MOUSE_BUTTON_LEFT)
			moving = action;
	});
}

void Freecam::updateCamera(gfx::Camera& _camera) {

	// Get framerate independence multiplier
	auto framerateScale = std::min(gfx::s_renderer->frameTime(), 0.1f);
	_camera.moveSpeed = 0.5f * framerateScale;

	offset.y() *= -1; // Y points down in window coords but up in the world

	if (moving)
		_camera.rotate(offset.x(), offset.y());
	offset = vec2(0.0f); // Lateral movement applied, reset

	_camera.roam({
		float(right) - float(left),
		0.0f,
		float(up) - float(down),
	});
	_camera.shift({0.0f, 0.0f, float(floating)});

}

}
