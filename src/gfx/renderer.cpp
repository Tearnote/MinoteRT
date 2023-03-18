#include "gfx/renderer.hpp"

#include <utility>

#include <vuk/CommandBuffer.hpp>
#include <vuk/RenderGraph.hpp>
#include <vuk/Context.hpp>

#include "stx/except.hpp"
#include "sys/vulkan.hpp"

namespace minote::gfx {

Renderer::Renderer():
	m_deviceResource(sys::s_vulkan->context, InflightFrames),
	m_multiFrameAllocator(m_deviceResource) {}

void Renderer::draw() {

	// Begin the frame
	sys::s_vulkan->context.next_frame();
	auto& frameResource = m_deviceResource.get_next_frame();
	auto frameAllocator = vuk::Allocator(frameResource);

	// Create a rendergraph
	auto rg = std::make_shared<vuk::RenderGraph>("simple");
	rg->attach_swapchain("swapchain", sys::s_vulkan->swapchain);
	rg->add_pass(vuk::Pass{
		.name = "screen clear",
		.resources = {
			"swapchain"_image >> vuk::eClear >> "swapchain_out",
		},
		.execute = [](vuk::CommandBuffer& cmd) {
			cmd.clear_image("swapchain", vuk::ClearColor(0.4f, 0.4f, 0.5f, 1.0f));
		},
	});

	// Acquire and present
	auto compiler = vuk::Compiler();
	auto erg = *compiler.link(stx::ptr_span(&rg), {});
	auto acquireBundle = *vuk::acquire_one(frameAllocator, sys::s_vulkan->swapchain);
	auto submitBundle = *vuk::execute_submit(frameAllocator, std::move(erg), std::move(acquireBundle));
	vuk::present_to_one(sys::s_vulkan->context, std::move(submitBundle));

}

}
