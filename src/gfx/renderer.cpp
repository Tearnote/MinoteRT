#include "gfx/renderer.hpp"

#include <utility>

#include <vuk/CommandBuffer.hpp>
#include <vuk/RenderGraph.hpp>
#include <vuk/Context.hpp>

#include "math.hpp"
#include "sys/vulkan.hpp"
#include "gfx/samplers.hpp"
#include "gfx/modules/pathtrace.hpp"
#include "gfx/modules/tonemap.hpp"

namespace minote::gfx {

using namespace math_literals;

Renderer::Renderer():
	m_deviceResource(sys::s_vulkan->context, InflightFrames),
	m_multiFrameAllocator(m_deviceResource) {}

void Renderer::draw(gfx::Camera const& _camera) {

	// Begin the frame
	sys::s_vulkan->context.next_frame();
	auto& frameResource = m_deviceResource.get_next_frame();
	auto frameAllocator = vuk::Allocator(frameResource);

	// Initial temporal resource values
	if (sys::s_vulkan->context.get_frame_count() == 1)
		m_prevCamera = _camera;

	// Create a rendergraph
	auto gbuffer = modules::primaryRays(uvec2{
		sys::s_vulkan->swapchain->extent.width,
	    sys::s_vulkan->swapchain->extent.height
	}, _camera, m_prevCamera);
	auto pathtraced = modules::secondaryRays(std::move(gbuffer), _camera);
	auto tonemapped = modules::tonemap(std::move(pathtraced));

	// Blit to swapchain
	auto rg = std::make_shared<vuk::RenderGraph>("main");
	rg->attach_in("tonemapped", tonemapped);
	rg->attach_swapchain("swapchain/blank", sys::s_vulkan->swapchain);
	rg->add_pass(vuk::Pass{
		.name = "swapchain blit",
		.resources = {
			"tonemapped"_image >> vuk::eTransferRead,
			"swapchain/blank"_image >> vuk::eTransferWrite >> "swapchain",
		},
		.execute = [](vuk::CommandBuffer& cmd) {
			auto srcSize = cmd.get_resource_image_attachment("tonemapped").value().extent.extent;
			auto dstSize = cmd.get_resource_image_attachment("swapchain/blank").value().extent.extent;
			cmd.blit_image("tonemapped", "swapchain/blank", vuk::ImageBlit{
				.srcSubresource = vuk::ImageSubresourceLayers{ .aspectMask = vuk::ImageAspectFlagBits::eColor },
				.srcOffsets = {vuk::Offset3D{0, 0, 0}, vuk::Offset3D{int(srcSize.width), int(srcSize.height), 1}},
				.dstSubresource = vuk::ImageSubresourceLayers{ .aspectMask = vuk::ImageAspectFlagBits::eColor },
				.dstOffsets = {vuk::Offset3D{0, 0, 0}, vuk::Offset3D{int(dstSize.width), int(dstSize.height), 1}} },
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
