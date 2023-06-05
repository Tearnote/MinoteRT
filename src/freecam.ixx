export module minote.freecam;

import <algorithm>;
import <imgui.h>;
import <GLFW/glfw3.h>;
import minote.renderer;
import minote.window;
import minote.camera;
import minote.math;

export class Freecam:
	Window,
	Renderer
{
public:
	Freecam() { prevCursorPos = Window::serv->getCursorPosition(); }

	// Register callbacks with GLFW
	void registerEvents() {
		Window::serv->registerKeyCallback([this](int key, bool action) {
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
		Window::serv->registerCursorMotionCallback([this](vec2 newPos) {
			offset += newPos - prevCursorPos;
			prevCursorPos = newPos;
		});
		Window::serv->registerMouseButtonCallback([this](int button, bool action) {
			if (action && ImGui::GetIO().WantCaptureMouse) return;
			if (button == GLFW_MOUSE_BUTTON_LEFT)
				moving = action;
		});
	}

	// Apply freecam to a camera
	void updateCamera(Camera& camera) {
		// Get framerate independence multiplier
		auto framerateScale = std::min(Renderer::serv->frameTime(), 0.1f);
		camera.moveSpeed = 0.0005f * framerateScale;

		offset.y() *= -1; // Y points down in window coords but up in the world

		if (moving)
			camera.rotate(offset.x(), offset.y());
		offset = vec2(0.0f); // Lateral movement applied, reset

		camera.roam({
			float(right) - float(left),
			0.0f,
			float(up) - float(down),
		});
		camera.shift({0.0f, 0.0f, float(floating)});
	}

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
