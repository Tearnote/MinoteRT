#include "gfx/renderer.hpp"

#include <utility>

#include <vuk/CommandBuffer.hpp>
#include <vuk/RenderGraph.hpp>
#include <vuk/Context.hpp>
#include <imgui.h>

#include "math.hpp"
#include "sys/glfw.hpp"
#include "sys/vulkan.hpp"
#include "gfx/modules/pathtrace.hpp"
#include "gfx/modules/tonemap.hpp"

namespace minote::gfx {

using namespace math_literals;

Renderer::Renderer():
	m_deviceResource(sys::s_vulkan->context, InflightFrames),
	m_multiFrameAllocator(m_deviceResource),
	m_imgui(m_multiFrameAllocator) {}

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

	// Calculate framerate
	m_framesSinceLastCheck += 1;
	auto currentTime = sys::s_glfw->getTime();
	auto timeElapsed = currentTime - m_lastFrameTimeCheck;
	if (timeElapsed >= FrameTimeUpdate) {
		auto secondsElapsed = stx::ratio(timeElapsed, 1_s);
		m_frameTime = secondsElapsed / float(m_framesSinceLastCheck);

		m_lastFrameTimeCheck = currentTime;
		m_framesSinceLastCheck = 0;
	}
	ImGui::Text("Frametime: %.1f ms", m_frameTime * 1000.0f);

	// Initial temporal resource values
	if (sys::s_vulkan->context.get_frame_count() == 1)
		m_prevCamera = _camera;

	// Create a rendergraph
	auto gbuffer = modules::primaryRays(outputSize, _camera, m_prevCamera);
	auto pathtraced = modules::secondaryRays(std::move(gbuffer), _camera);

	enum class TonemapMode: int {
		Linear = 0,
		Uchimura = 1,
	};
	constexpr static auto TonemapModeStrings = std::to_array<const char*>({
	    "Linear",
		"Uchimura",
	});
	static auto exposure = 1.0f;
	static auto tonemapMode = TonemapMode::Uchimura;
	if (ImGui::CollapsingHeader("Tonemapper")) {
		ImGui::SliderFloat("Exposure", &exposure, 0.1f, 10.0f, "%.1f",
						   ImGuiSliderFlags_NoRoundToFormat | ImGuiSliderFlags_Logarithmic);
		ImGui::Combo("Algorithm", reinterpret_cast<int*>(&tonemapMode),
		             TonemapModeStrings.data(), TonemapModeStrings.size());
	}
	auto tonemapped = [&]() {
		switch (tonemapMode) {
			case TonemapMode::Linear:
				return modules::tonemapLinear(std::move(pathtraced), exposure);
			case TonemapMode::Uchimura:
				return modules::tonemapUchimura(std::move(pathtraced), exposure, modules::UchimuraParams::make_default());
		}
	}();

	auto imgui = m_imgui.render(frameAllocator, std::move(tonemapped));

	// Blit to swapchain
	auto rg = std::make_shared<vuk::RenderGraph>("main");
	rg->attach_in("imgui", std::move(imgui));
	rg->attach_swapchain("swapchain/blank", sys::s_vulkan->swapchain);
	rg->add_pass(vuk::Pass{
		.name = "swapchain blit",
		.resources = {
			"imgui"_image >> vuk::eTransferRead,
			"swapchain/blank"_image >> vuk::eTransferWrite >> "swapchain",
		},
		.execute = [outputSize](vuk::CommandBuffer& cmd) {
			cmd.blit_image("imgui", "swapchain/blank", vuk::ImageBlit{
				.srcSubresource = vuk::ImageSubresourceLayers{ .aspectMask = vuk::ImageAspectFlagBits::eColor },
				.srcOffsets = {vuk::Offset3D{0, 0, 0}, vuk::Offset3D{int(outputSize.x()), int(outputSize.y()), 1}},
				.dstSubresource = vuk::ImageSubresourceLayers{ .aspectMask = vuk::ImageAspectFlagBits::eColor },
				.dstOffsets = {vuk::Offset3D{0, 0, 0}, vuk::Offset3D{int(outputSize.x()), int(outputSize.y()), 1}} },
			vuk::Filter::eNearest);
		},
	});

	// Acquire, submit and present
	auto compiler = vuk::Compiler();
	auto erg = *compiler.link(stx::ptr_span(&rg), {});
	auto acquireBundle = *vuk::acquire_one(frameAllocator, sys::s_vulkan->swapchain);
	auto submitBundle = *vuk::execute_submit(frameAllocator, std::move(erg), std::move(acquireBundle));
	vuk::present_to_one(sys::s_vulkan->context, std::move(submitBundle));

	// Temporal preservation
	m_prevCamera = _camera;

}

}
