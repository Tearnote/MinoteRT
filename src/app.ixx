export module minote.app;

import <exception>;
import <cstdlib>;
import minote.renderer;
import minote.freecam;
import minote.vulkan;
import minote.camera;
import minote.window;
import minote.log;

export class App:
	Window,
	Vulkan,
	Renderer,
	Log
{
public:
	auto run() -> int try {
		auto camera = Camera{
			.viewport = {
				Vulkan::serv->swapchain->extent.width,
				Vulkan::serv->swapchain->extent.height,
			},
			.verticalFov = 60_deg,
			.nearPlane = 0.001f,
			.position = {0.0f, -0.001f, 0.1f},
			.yaw = 90_deg,
			.pitch = 0.0f,
			.lookSpeed = 1.0f / 256.0f,
			.moveSpeed = 8.0f,
		};
		auto freecam = Freecam();
		freecam.registerEvents();

		while (!Window::serv->isClosing()) {
			Window::serv->poll();
			freecam.updateCamera(camera);
			Renderer::serv->draw(camera);
		}
		return EXIT_SUCCESS;
	} catch (std::exception e) {
		error("Uncaught exception on main thread: {}", e.what());
		return EXIT_FAILURE;
	}
};
