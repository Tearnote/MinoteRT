export module minote.window;

import <functional>;
import <utility>;
import <cassert>;
import <vector>;
import <memory>;
import <string>;
import <print>;
import <GLFW/glfw3.h>;
import minote.service;
import minote.math;
import minote.time;
import minote.log;

class Window_impl:
	Log
{
public:
	auto isClosing() -> bool { return glfwWindowShouldClose(windowPtr.get()); }

	void poll() { glfwPollEvents(); }

	auto size() -> uvec2 {
		auto w = 0;
		auto h = 0;
		glfwGetFramebufferSize(windowPtr.get(), &w, &h);
		return uvec2{uint(w), uint(h)};
	}

	auto getCursorPosition() -> vec2 {
		auto x = 0.0;
		auto y = 0.0;
		glfwGetCursorPos(windowPtr.get(), &x, &y);
		return vec2{float(x), float(y)};
	}

	void registerKeyCallback(std::function<void(int, bool)> func) {
		keyCallbacks.emplace_back(std::move(func));
	}

	void registerCursorMotionCallback(std::function<void(vec2)> func) {
		cursorMotionCallbacks.emplace_back(std::move(func));
	}

	void registerMouseButtonCallback(std::function<void(int, bool)> func) {
		mouseButtonCallbacks.emplace_back(std::move(func));
	}

	auto getTime() -> nsec {
		return seconds(glfwGetTime());
	}

	auto handle() -> GLFWwindow* {
		return windowPtr.get();
	}

private:
	// Only usable from the service
	friend class Service<Window_impl>;

	Window_impl(std::string const& title, uvec2 size) {
		assert(size.x() > 0 && size.y() > 0);

		// Create the window
		glfwSetErrorCallback([](int code, char const* str) {
			std::print(stderr, "[GLFW] Error {}: {}", code, str);
		});
		glfwInit();
		info("GLFW initialized");

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		windowPtr = GlfwWindowPtr(glfwCreateWindow(size.x(), size.y(), title.c_str(), nullptr, nullptr));
		assert(windowPtr);

		// Set up event callbacks
		glfwSetWindowUserPointer(windowPtr.get(), this);

		glfwSetKeyCallback(windowPtr.get(), [](GLFWwindow* windowPtr, int key, int, int action, int) {
			if (action == GLFW_REPEAT) return;
			auto& window = *reinterpret_cast<Window_impl*>(glfwGetWindowUserPointer(windowPtr));
			for (auto& func: window.keyCallbacks)
				func(key, action == GLFW_PRESS);
		});

		glfwSetCursorPosCallback(windowPtr.get(), [](GLFWwindow* windowPtr, double x, double y) {
			auto& window = *reinterpret_cast<Window_impl*>(glfwGetWindowUserPointer(windowPtr));
			for (auto& func : window.cursorMotionCallbacks)
				func(vec2{float(x), float(y)});
			});

		glfwSetMouseButtonCallback(windowPtr.get(), [](GLFWwindow* windowPtr, int button, int action, int) {
			auto& window = *reinterpret_cast<Window_impl*>(glfwGetWindowUserPointer(windowPtr));
			for (auto& func: window.mouseButtonCallbacks)
				func(button, action == GLFW_PRESS);
		});

		// Quit on ESC
		registerKeyCallback([this](int key, bool) {
			if (key == GLFW_KEY_ESCAPE)
				glfwSetWindowShouldClose(windowPtr.get(), GLFW_TRUE);
		});

		info("Created window \"{}\" at {}x{}", title, size.x(), size.y());
	}

	~Window_impl() {
		if (!windowPtr) return;

		windowPtr.reset();
		glfwTerminate();
		info("Window closed");
	}

	// Moveable
	Window_impl(Window_impl&&) = default;
	auto operator=(Window_impl&&) -> Window_impl& = default;

	using GlfwWindowPtr = std::unique_ptr<GLFWwindow, decltype([](auto* w) {
		glfwDestroyWindow(w);
	})>;

	GlfwWindowPtr windowPtr;

	std::vector<std::function<void(int, bool)>> keyCallbacks;
	std::vector<std::function<void(vec2)>> cursorMotionCallbacks;
	std::vector<std::function<void(int, bool)>> mouseButtonCallbacks;
};

export using Window = Service<Window_impl>;
