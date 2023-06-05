#ifndef NOMINMAX
#define NOMINMAX
#endif //NOMINMAX
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif //WIN32_LEAN_AND_MEAN
#include <Windows.h>

import "config.hpp";
import minote.renderer;
import minote.window;
import minote.vulkan;
import minote.time;
import minote.math;
import minote.log;
import minote.app;
import minote.os;

auto init() {
	setThreadName("main");
	createConsole();
	struct Services {
		Logger::Provider logger{LoggingLevel, LogfilePath};
		Window::Provider window{AppTitle, uvec2{960, 540}};
		Vulkan::Provider vulkan{};
		Renderer::Provider renderer{};
		SchedulerPeriod period{1_ms};
	};
	return Services();
}

auto WinMain(HINSTANCE, HINSTANCE, LPSTR, int) -> int {
	auto services = init();
	auto app = App();
	return app.run();
}
