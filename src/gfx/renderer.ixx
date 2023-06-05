export module minote.renderer;

import <utility>;
import <vector>;
import <memory>;
import <array>;
import <lodepng.h>;
import <imgui.h>;
import <vuk/resources/DeviceFrameResource.hpp>;
import <vuk/CommandBuffer.hpp>;
import <vuk/RenderGraph.hpp>;
import <vuk/Allocator.hpp>;
import <vuk/Partials.hpp>;
import <vuk/Context.hpp>;
import minote.modules.pathtracer;
import minote.modules.tonemapper;
import minote.modules.denoiser;
import minote.modules.sky;
import minote.samplers;
import minote.window;
import minote.ranges;
import minote.except;
import minote.vulkan;
import minote.camera;
import minote.imgui;
import minote.math;
import minote.time;
import minote.log;

class Renderer_impl:
	Log,
	Window,
	Vulkan
{
public:
	constexpr static auto InflightFrames = 3u;
	static constexpr auto FrameTimeUpdate = 0.25_s;

	void draw(Camera const& camera) {
		// Begin the frame
		Vulkan::serv->context.next_frame();
		auto& frameResource = deviceResource.get_next_frame();
		auto frameAllocator = vuk::Allocator(frameResource);
		auto outputSize = uvec2{
			Vulkan::serv->swapchain->extent.width,
			Vulkan::serv->swapchain->extent.height
		};
		imgui.begin(outputSize);
		updateFrameTime();

		// Initial temporal resource values
		if (Vulkan::serv->context.get_frame_count() == 1)
			prevCamera = camera;

		// Create a rendergraph
		auto atmosphere = Atmosphere(frameAllocator, Atmosphere::Params::earth());
		auto sky = Sky();
		auto skyView = sky.createView(atmosphere, camera.position);
		auto gbuffer = pathtracer.primaryRays(outputSize, camera, prevCamera);
		auto pathtraced = pathtracer.secondaryRays(gbuffer, camera, atmosphere, skyView, blueNoise);
		auto filtered = denoise(std::move(pathtraced), gbuffer.depth, gbuffer.normal, camera);
		auto tonemapped = tonemap(std::move(filtered));
		auto withImgui = imgui.render(frameAllocator, std::move(tonemapped));
		blitAndPresent(std::move(withImgui), frameAllocator);

		// Temporal preservation
		prevCamera = camera;
	}

	[[nodiscard]]
	auto frameTime() const -> float { return m_frameTime; }

	// Not movable, not copyable
	Renderer_impl(Renderer_impl const&) = delete;
	auto operator=(Renderer_impl const&) -> Renderer_impl& = delete;

private:
	vuk::DeviceSuperFrameResource deviceResource;
	vuk::Allocator multiFrameAllocator;
	Camera prevCamera;
	Imgui imgui;

	float m_frameTime;
	nsec lastFrameTimeCheck;
	uint framesSinceLastCheck;

	vuk::Texture blueNoise;

	Denoiser denoiser;
	Pathtracer pathtracer;
	Tonemapper tonemapper;

	// Can only be used as service
	friend Service<Renderer_impl>;

	Renderer_impl():
		deviceResource(Vulkan::serv->context, InflightFrames),
		multiFrameAllocator(deviceResource),
		imgui(multiFrameAllocator)
	{
		auto pixels = std::vector<unsigned char>();
		auto pixelsSize = uvec2();
		lodepng::decode(pixels, pixelsSize.x(), pixelsSize.y(), "assets/blue_noise.png", LCT_RGBA, 8);
		auto [pixelsTex, stub] = vuk::create_texture(multiFrameAllocator,
			vuk::Format::eR8G8B8A8Unorm, vuk::Extent3D{pixelsSize.x(), pixelsSize.y(), 1u},
			pixels.data(), false
		);
		blueNoise = std::move(pixelsTex);
		auto comp = vuk::Compiler();
		stub.wait(multiFrameAllocator, comp);
	}

	void updateFrameTime() {
		framesSinceLastCheck += 1;
		auto currentTime = Window::serv->getTime();
		auto timeElapsed = currentTime - lastFrameTimeCheck;
		if (timeElapsed >= FrameTimeUpdate) {
			auto secondsElapsed = ratio(timeElapsed, 1_s);
			m_frameTime = secondsElapsed / float(framesSinceLastCheck);

			lastFrameTimeCheck = currentTime;
			framesSinceLastCheck = 0;
		}
		ImGui::Text("Frame time: %.2f ms", m_frameTime * 1000.0f);
	}

	auto denoise(vuk::Future color, vuk::Future depth, vuk::Future normal, Camera const& camera) -> vuk::Future {
		// Tonemapper selection
		enum class DenoiseMode: int {
			None = 0,
			Bilateral = 1,
		};
		constexpr static auto DenoiseModeStrings = std::to_array<const char*>({
			"None",
			"Bilateral",
		});

		// Expose all controls via Imgui
		static auto denoiseMode = DenoiseMode::Bilateral;
		static auto bilateralParams = BilateralParams::make_default();
		if (ImGui::CollapsingHeader("Denoiser")) {
			ImGui::Combo("Algorithm", reinterpret_cast<int*>(&denoiseMode),
				DenoiseModeStrings.data(), DenoiseModeStrings.size()
			);
			if (denoiseMode == DenoiseMode::Bilateral) {
				ImGui::SliderFloat("Sigma", &bilateralParams.sigma, 1.0f, 10.0f, "%.2f");
				ImGui::SliderFloat("kSigma", &bilateralParams.kSigma, 1.0f, 3.0f, "%.2f");
				ImGui::SliderFloat("Threshold", &bilateralParams.threshold, 0.01f, 1.0f, "%.2f");
			}
		}

		// Perform denoising via chosen method
		switch (denoiseMode) {
		case DenoiseMode::None:
			return std::move(color);
		case DenoiseMode::Bilateral:
			return denoiser.bilateral(std::move(color), std::move(depth), std::move(normal), camera, bilateralParams);
		default:
			throw logic_error_fmt("Unknown denoise mode {}", std::to_underlying(denoiseMode));
		}
	}

	auto tonemap(vuk::Future color) -> vuk::Future {
		// Tonemapper selection
		enum class TonemapMode: int {
			Linear = 0,
			Reinhard = 1,
			Hable = 2,
			ACES = 3,
			Uchimura = 4,
			AMD = 5,
		};
		constexpr static auto TonemapModeStrings = std::to_array<const char*>({
			"Linear",
			"Reinhard",
			"Hable",
			"ACES",
			"Uchimura",
			"AMD",
		});

		// Expose all controls via Imgui
		static auto exposure = 1.0f;
		static auto tonemapMode = TonemapMode::AMD;
		static auto reinhardMax = 8.0f;
		static auto uchimuraParams = UchimuraParams::make_default();
		static auto amdParams = AMDParams::make_default();
		if (ImGui::CollapsingHeader("Tonemapper")) {
			ImGui::SliderFloat("Exposure", &exposure, 0.1f, 10.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
			ImGui::Combo("Algorithm", reinterpret_cast<int*>(&tonemapMode),
				TonemapModeStrings.data(), TonemapModeStrings.size()
			);
			if (tonemapMode == TonemapMode::Reinhard) {
				ImGui::SliderFloat("HDR peak", &reinhardMax, 1.0f, 32.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
			}
			if (tonemapMode == TonemapMode::Uchimura) {
				ImGui::SliderFloat("Max brightness", &uchimuraParams.maxBrightness, 1.0f, 10.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
				ImGui::SliderFloat("Contrast", &uchimuraParams.contrast, 0.1f, 2.4f, "%.2f");
				ImGui::SliderFloat("Linear start", &uchimuraParams.linearStart, 0.01f, 0.9f, "%.2f");
				ImGui::SliderFloat("Linear length", &uchimuraParams.linearLength, 0.0f, 0.9f, "%.2f");
				ImGui::SliderFloat("Black tightness", &uchimuraParams.blackTightness, 1.0f, 3.0f, "%.2f");
				ImGui::SliderFloat("Pedestal", &uchimuraParams.pedestal, 0.0f, 1.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
			}
			if (tonemapMode == TonemapMode::AMD) {
				ImGui::SliderFloat("HDR peak", &amdParams.hdrMax, 1.0f, 32.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
				ImGui::SliderFloat("Contrast", &amdParams.contrast, 0.5f, 4.0f, "%.2f");
				ImGui::SliderFloat("Shoulder", &amdParams.shoulder, 0.9f, 1.0f, "%.2f");
				ImGui::SliderFloat("Mid in", &amdParams.midIn, 0.01f, 1.0f, "%.2f");
				ImGui::SliderFloat("Mid out", &amdParams.midOut, 0.01f, 0.99f, "%.2f");
			}
		}

		// Perform tonemapping via chosen method
		switch (tonemapMode) {
		case TonemapMode::Linear:
			return tonemapper.linear(std::move(color), exposure);
		case TonemapMode::Reinhard:
			return tonemapper.reinhard(std::move(color), exposure, reinhardMax);
		case TonemapMode::Hable:
			return tonemapper.hable(std::move(color), exposure);
		case TonemapMode::ACES:
			return tonemapper.aces(std::move(color), exposure);
		case TonemapMode::Uchimura:
			return tonemapper.uchimura(std::move(color), exposure, uchimuraParams);
		case TonemapMode::AMD:
			return tonemapper.amd(std::move(color), exposure, amdParams);
		default:
			throw logic_error_fmt("Unknown tonemap mode {}", std::to_underlying(tonemapMode));
		}
	}

	void blitAndPresent(vuk::Future color, vuk::Allocator& allocator) {
		// Blit to swapchain
		auto rg = std::make_shared<vuk::RenderGraph>("swapchain");
		rg->attach_in("color", std::move(color));
		rg->attach_swapchain("swapchain/blank", Vulkan::serv->swapchain);
		rg->add_pass(vuk::Pass{
			.name = "swapchain blit",
			.resources = {
				"color"_image >> vuk::eTransferRead,
				"swapchain/blank"_image >> vuk::eTransferWrite >> "swapchain",
			},
			.execute = [](auto& cmd) {
				auto size = cmd.get_resource_image_attachment("swapchain/blank")->extent.extent;
				cmd.blit_image("color", "swapchain/blank",
					vuk::ImageBlit{
						.srcSubresource = vuk::ImageSubresourceLayers{.aspectMask = vuk::ImageAspectFlagBits::eColor },
						.srcOffsets = {vuk::Offset3D{0, 0, 0}, vuk::Offset3D{int(size.width), int(size.height), 1}},
						.dstSubresource = vuk::ImageSubresourceLayers{.aspectMask = vuk::ImageAspectFlagBits::eColor },
						.dstOffsets = {vuk::Offset3D{0, 0, 0}, vuk::Offset3D{int(size.width), int(size.height), 1}}
					},
					vuk::Filter::eNearest
				);
			},
			});

		// Acquire, submit and present
		auto compiler = vuk::Compiler();
		auto erg = *compiler.link(ptr_span(&rg), {});
		auto acquireBundle = *vuk::acquire_one(allocator, Vulkan::serv->swapchain);
		auto submitBundle = *vuk::execute_submit(allocator, std::move(erg), std::move(acquireBundle));
		vuk::present_to_one(Vulkan::serv->context, std::move(submitBundle));
	}
};

export using Renderer = Service<Renderer_impl>;
