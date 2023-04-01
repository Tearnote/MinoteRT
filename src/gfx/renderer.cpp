#include "gfx/renderer.hpp"

#include <utility>

#include <vuk/CommandBuffer.hpp>
#include <vuk/RenderGraph.hpp>
#include <vuk/Partials.hpp>
#include <vuk/Context.hpp>
#include <lodepng.h>
#include <imgui.h>

#include "math.hpp"
#include "stx/except.hpp"
#include "stx/enum.hpp"
#include "sys/glfw.hpp"
#include "sys/vulkan.hpp"
#include "gfx/modules/pathtrace.hpp"
#include "gfx/modules/denoise.hpp"
#include "gfx/modules/tonemap.hpp"

namespace minote::gfx {

using namespace math_literals;
using namespace stx::enum_literals;

Renderer::Renderer():
	m_deviceResource(sys::s_vulkan->context, InflightFrames),
	m_multiFrameAllocator(m_deviceResource),
	m_imgui(m_multiFrameAllocator) {

	auto pixels = std::vector<unsigned char>();
	auto pixelsSize = uvec2();
	lodepng::decode(pixels, pixelsSize.x(), pixelsSize.y(), "assets/blue_noise.png", LCT_RGBA, 8);
	auto [pixelsTex, stub] = vuk::create_texture(m_multiFrameAllocator,
		vuk::Format::eR8G8B8A8Unorm, vuk::Extent3D{pixelsSize.x(), pixelsSize.y(), 1u},
		pixels.data(), false);
	m_blueNoise = std::move(pixelsTex);
	auto comp = vuk::Compiler();
	stub.wait(m_multiFrameAllocator, comp);

}

void Renderer::draw(gfx::Camera const& _camera) {

	// Begin the frame
	sys::s_vulkan->context.next_frame();
	auto& frameResource = m_deviceResource.get_next_frame();
	auto frameAllocator = vuk::Allocator(frameResource);
	auto outputSize = uvec2{
		sys::s_vulkan->swapchain->extent.width,
		sys::s_vulkan->swapchain->extent.height
	};
	m_imgui.begin(outputSize);
	updateFrameTime();

	// Initial temporal resource values
	if (sys::s_vulkan->context.get_frame_count() == 1)
		m_prevCamera = _camera;

	// Create a rendergraph
	auto gbuffer = modules::primaryRays(outputSize, _camera, m_prevCamera);
	auto pathtraced = modules::secondaryRays(gbuffer, _camera, m_blueNoise);
	auto filtered = denoise(std::move(pathtraced), gbuffer.depth);
	auto tonemapped = tonemap(std::move(filtered));
	auto imgui = m_imgui.render(frameAllocator, std::move(tonemapped));
	blitAndPresent(std::move(imgui), frameAllocator);

	// Temporal preservation
	m_prevCamera = _camera;

}

void Renderer::updateFrameTime() {

	m_framesSinceLastCheck += 1;
	auto currentTime = sys::s_glfw->getTime();
	auto timeElapsed = currentTime - m_lastFrameTimeCheck;
	if (timeElapsed >= FrameTimeUpdate) {
		auto secondsElapsed = stx::ratio(timeElapsed, 1_s);
		m_frameTime = secondsElapsed / float(m_framesSinceLastCheck);

		m_lastFrameTimeCheck = currentTime;
		m_framesSinceLastCheck = 0;
	}
	ImGui::Text("Frame time: %.2f ms", m_frameTime * 1000.0f);

}

auto Renderer::denoise(vuk::Future _color, vuk::Future _depth) -> vuk::Future {

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
	static auto denoiseMode = DenoiseMode::None;
	static auto bilateralParams = modules::BilateralParams::make_default();
	if (ImGui::CollapsingHeader("Denoiser")) {
		ImGui::Combo("Algorithm", reinterpret_cast<int*>(&denoiseMode),
			DenoiseModeStrings.data(), DenoiseModeStrings.size());
		if (denoiseMode == DenoiseMode::Bilateral) {
			ImGui::SliderFloat("Sigma", &bilateralParams.sigma, 1.0f, 10.0f, "%.2f");
			ImGui::SliderFloat("kSigma", &bilateralParams.kSigma, 1.0f, 3.0f, "%.2f");
			ImGui::SliderFloat("Threshold", &bilateralParams.threshold, 0.01f, 1.0f, "%.2f");
		}
	}

	// Perform tonemapping via chosen method
	switch (denoiseMode) {
		case DenoiseMode::None:
			return std::move(_color);
		case DenoiseMode::Bilateral:
			return modules::denoiseBilateral(std::move(_color), std::move(_depth), bilateralParams);
		default:
			throw stx::logic_error_fmt("Unknown denoise mode {}", +denoiseMode);
	}

}

auto Renderer::tonemap(vuk::Future _input) -> vuk::Future {

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
	static auto uchimuraParams = modules::UchimuraParams::make_default();
	static auto amdParams = modules::AMDParams::make_default();
	if (ImGui::CollapsingHeader("Tonemapper")) {
		ImGui::SliderFloat("Exposure", &exposure, 0.1f, 10.0f, "%.2f", ImGuiSliderFlags_Logarithmic);
		ImGui::Combo("Algorithm", reinterpret_cast<int*>(&tonemapMode),
			TonemapModeStrings.data(), TonemapModeStrings.size());
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
			return modules::tonemapLinear(std::move(_input), exposure);
		case TonemapMode::Reinhard:
			return modules::tonemapReinhard(std::move(_input), exposure, reinhardMax);
		case TonemapMode::Hable:
			return modules::tonemapHable(std::move(_input), exposure);
		case TonemapMode::ACES:
			return modules::tonemapAces(std::move(_input), exposure);
		case TonemapMode::Uchimura:
			return modules::tonemapUchimura(std::move(_input), exposure, uchimuraParams);
		case TonemapMode::AMD:
			return modules::tonemapAMD(std::move(_input), exposure, amdParams);
		default:
			throw stx::logic_error_fmt("Unknown tonemap mode {}", +tonemapMode);
	}

}

void Renderer::blitAndPresent(vuk::Future _source, vuk::Allocator& _allocator) {

	// Blit to swapchain
	auto rg = std::make_shared<vuk::RenderGraph>("swapchain");
	rg->attach_in("source", std::move(_source));
	rg->attach_swapchain("swapchain/blank", sys::s_vulkan->swapchain);
	rg->add_pass(vuk::Pass{
		.name = "swapchain blit",
		.resources = {
			"source"_image >> vuk::eTransferRead,
			"swapchain/blank"_image >> vuk::eTransferWrite >> "swapchain",
		},
		.execute = [](vuk::CommandBuffer& cmd) {
			auto size = cmd.get_resource_image_attachment("swapchain/blank")->extent.extent;
			cmd.blit_image("source", "swapchain/blank", vuk::ImageBlit{
				               .srcSubresource = vuk::ImageSubresourceLayers{ .aspectMask = vuk::ImageAspectFlagBits::eColor },
				               .srcOffsets = {vuk::Offset3D{0, 0, 0}, vuk::Offset3D{int(size.width), int(size.height), 1}},
				               .dstSubresource = vuk::ImageSubresourceLayers{ .aspectMask = vuk::ImageAspectFlagBits::eColor },
				               .dstOffsets = {vuk::Offset3D{0, 0, 0}, vuk::Offset3D{int(size.width), int(size.height), 1}} },
			               vuk::Filter::eNearest);
		},
	});

	// Acquire, submit and present
	auto compiler = vuk::Compiler();
	auto erg = *compiler.link(stx::ptr_span(&rg), {});
	auto acquireBundle = *vuk::acquire_one(_allocator, sys::s_vulkan->swapchain);
	auto submitBundle = *vuk::execute_submit(_allocator, std::move(erg), std::move(acquireBundle));
	vuk::present_to_one(sys::s_vulkan->context, std::move(submitBundle));

}

}
