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

void Renderer::draw(gfx::Camera const& _camera) {

	// Begin the frame
	sys::s_vulkan->context.next_frame();
	auto& frameResource = m_deviceResource.get_next_frame();
	auto frameAllocator = vuk::Allocator(frameResource);

	// Compile shaders
	static auto shadersCompiled = false;
	if(!shadersCompiled) {
		auto shaderPci = vuk::PipelineBaseCreateInfo();
		constexpr auto shaderSource = std::to_array<uint>({
#include "spv/shader.comp.spv"
		                                            });
		shaderPci.add_static_spirv(shaderSource.data(), shaderSource.size(), "shader.comp");
		sys::s_vulkan->context.create_named_pipeline("shader_comp", shaderPci);

		auto denoisePci = vuk::PipelineBaseCreateInfo();
		constexpr auto denoiseSource = std::to_array<uint>({
#include "spv/denoise.comp.spv"
		                                            });
		denoisePci.add_static_spirv(denoiseSource.data(), denoiseSource.size(), "denoise.comp");
		sys::s_vulkan->context.create_named_pipeline("denoise", denoisePci);
	}

	// Create a rendergraph
	auto rg = std::make_shared<vuk::RenderGraph>("simple");
	rg->attach_image("rt/blank", vuk::ImageAttachment{
		.extent = vuk::Dimension3D::absolute(
			sys::s_vulkan->swapchain->extent.width,
			sys::s_vulkan->swapchain->extent.height
		),
		.format = vuk::Format::eR16G16B16A16Sfloat,
		.sample_count = vuk::Samples::e1,
		.level_count = 1,
		.layer_count = 1,
	});
	rg->attach_image("denoised/blank", vuk::ImageAttachment{
		.extent = vuk::Dimension3D::absolute(
			sys::s_vulkan->swapchain->extent.width,
			sys::s_vulkan->swapchain->extent.height
		),
		.format = vuk::Format::eR16G16B16A16Sfloat,
		.sample_count = vuk::Samples::e1,
		.level_count = 1,
		.layer_count = 1,
	});
	rg->attach_swapchain("swapchain", sys::s_vulkan->swapchain);
	rg->add_pass(vuk::Pass{
		.name = "raytracing",
		.resources = {
			"rt/blank"_image >> vuk::eComputeWrite >> "rt/out",
		},
		.execute = [&_camera](vuk::CommandBuffer& cmd) {
			cmd.bind_compute_pipeline("shader_comp")
				.bind_image(0, 0, "rt/blank");

			struct Constants {
				mat4 View;
				uint FrameCounter;
			};
			cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0, Constants{
				.View = _camera.view(),
				.FrameCounter = uint(sys::s_vulkan->context.get_frame_count()),
			});

			cmd.dispatch_invocations(
				sys::s_vulkan->swapchain->extent.width,
				sys::s_vulkan->swapchain->extent.height
			);
		},
	});
	rg->add_pass(vuk::Pass{
		.name = "denoising",
		.resources = {
			"rt/out"_image >> vuk::eComputeSampled,
			"denoised/blank"_image >> vuk::eComputeWrite >> "denoised/out",
		},
		.execute = [](vuk::CommandBuffer& cmd) {
			cmd.bind_compute_pipeline("denoise")
				.bind_image(0, 0, "rt/out").bind_sampler(0, 0, vuk::SamplerCreateInfo{
					.magFilter = vuk::Filter::eLinear,
					.minFilter = vuk::Filter::eLinear,
					.addressModeU = vuk::SamplerAddressMode::eClampToEdge,
					.addressModeV = vuk::SamplerAddressMode::eClampToEdge,
				})
				.bind_image(0, 1, "denoised/blank");

			cmd.dispatch_invocations(
				sys::s_vulkan->swapchain->extent.width,
				sys::s_vulkan->swapchain->extent.height
			);
		},
	});
	rg->add_pass(vuk::Pass{
		.name = "swapchain blit",
		.resources = {
			"denoised/out"_image >> vuk::eTransferRead,
			"swapchain"_image >> vuk::eTransferWrite >> "swapchain/out",
		},
		.execute = [](vuk::CommandBuffer& cmd) {
			auto srcSize = cmd.get_resource_image_attachment("denoised/out").value().extent.extent;
			auto dstSize = cmd.get_resource_image_attachment("swapchain").value().extent.extent;
			cmd.blit_image("denoised/out", "swapchain", vuk::ImageBlit{
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
