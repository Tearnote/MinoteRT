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

	// Compile shaders
	static auto shaderCompiled = false;
	if(!shaderCompiled) {
		auto pci = vuk::PipelineBaseCreateInfo();
		constexpr auto source = std::to_array<uint>({
#include "spv/shader.comp.spv"
		});
		pci.add_static_spirv(source.data(), source.size(), "shader.comp");
		sys::s_vulkan->context.create_named_pipeline("shader_comp", pci);
	}

	// Create a rendergraph
	auto rg = std::make_shared<vuk::RenderGraph>("simple");
	rg->attach_image("image", vuk::ImageAttachment{
		.extent = vuk::Dimension3D::absolute(
			sys::s_vulkan->swapchain->extent.width,
			sys::s_vulkan->swapchain->extent.height
		),
		.format = vuk::Format::eR8G8B8A8Unorm,
		.sample_count = vuk::Samples::e1,
		.level_count = 1,
		.layer_count = 1,
	});
	rg->attach_swapchain("swapchain", sys::s_vulkan->swapchain);
	rg->add_pass(vuk::Pass{
		.name = "screen clear",
		.resources = {
			"image"_image >> vuk::eComputeWrite >> "image_out",
		},
		.execute = [](vuk::CommandBuffer& cmd) {
			cmd.bind_compute_pipeline("shader_comp")
			   .bind_image(0, 0, "image");

			struct Constants {
				vec3 CameraPos;
				uint FrameCounter;
				vec3 CameraDir;
			};
			cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0, Constants{
				.CameraPos = vec3{0, 0, 0},
				.FrameCounter = uint(sys::s_vulkan->context.get_frame_count()),
				.CameraDir = vec3{0, 1, 0},
			});

			cmd.dispatch_invocations(
				sys::s_vulkan->swapchain->extent.width,
				sys::s_vulkan->swapchain->extent.height
			);
		},
	});
	rg->add_pass(vuk::Pass{
		.name = "swapchain blit",
		.resources = {
			"image_out"_image >> vuk::eTransferRead,
			"swapchain"_image >> vuk::eTransferWrite >> "swapchain_out",
		},
		.execute = [](vuk::CommandBuffer& cmd) {
			auto srcSize = cmd.get_resource_image_attachment("image_out").value().extent.extent;
			auto dstSize = cmd.get_resource_image_attachment("swapchain").value().extent.extent;
			cmd.blit_image("image_out", "swapchain", vuk::ImageBlit{
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

}

}
