#include "gfx/renderer.hpp"

#include <utility>

#include <vuk/CommandBuffer.hpp>
#include <vuk/RenderGraph.hpp>
#include <vuk/Context.hpp>

#include "math.hpp"
#include "sys/vulkan.hpp"

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

	// Compile shaders
	static auto shadersCompiled = false;
	if(!shadersCompiled) {
		auto primaryRayPci = vuk::PipelineBaseCreateInfo();
		constexpr auto primaryRaySource = std::to_array<uint>({
#include "spv/primaryRay.comp.spv"
		});
		primaryRayPci.add_static_spirv(primaryRaySource.data(), primaryRaySource.size(), "primaryRay.comp");
		sys::s_vulkan->context.create_named_pipeline("primary_ray", primaryRayPci);

		auto tonemapPci = vuk::PipelineBaseCreateInfo();
		constexpr auto tonemapSource = std::to_array<uint>({
#include "spv/tonemap.comp.spv"
		});
		tonemapPci.add_static_spirv(tonemapSource.data(), tonemapSource.size(), "tonemap.comp");
		sys::s_vulkan->context.create_named_pipeline("tonemap", tonemapPci);
	}

	// Create a rendergraph
	auto rg = std::make_shared<vuk::RenderGraph>("simple");
	rg->attach_image("visibility/blank", vuk::ImageAttachment{
		.extent = vuk::Dimension3D::absolute(
			sys::s_vulkan->swapchain->extent.width,
			sys::s_vulkan->swapchain->extent.height
		),
		.format = vuk::Format::eR32Uint,
		.sample_count = vuk::Samples::e1,
		.level_count = 1,
		.layer_count = 1,
	});
	rg->attach_image("depth/blank", vuk::ImageAttachment{
		.extent = vuk::Dimension3D::absolute(
			sys::s_vulkan->swapchain->extent.width,
			sys::s_vulkan->swapchain->extent.height
		),
		.format = vuk::Format::eR16Sfloat,
		.sample_count = vuk::Samples::e1,
		.level_count = 1,
		.layer_count = 1,
	});
	rg->attach_image("normal/blank", vuk::ImageAttachment{
		.extent = vuk::Dimension3D::absolute(
			sys::s_vulkan->swapchain->extent.width,
			sys::s_vulkan->swapchain->extent.height
		),
		.format = vuk::Format::eR16G16B16A16Sfloat, // w unused
		.sample_count = vuk::Samples::e1,
		.level_count = 1,
		.layer_count = 1,
	});
	rg->attach_image("seed/blank", vuk::ImageAttachment{
		.extent = vuk::Dimension3D::absolute(
			sys::s_vulkan->swapchain->extent.width,
			sys::s_vulkan->swapchain->extent.height
		),
		.format = vuk::Format::eR32Uint,
		.sample_count = vuk::Samples::e1,
		.level_count = 1,
		.layer_count = 1,
	});
	rg->attach_image("motion/blank", vuk::ImageAttachment{
		.extent = vuk::Dimension3D::absolute(
			sys::s_vulkan->swapchain->extent.width,
			sys::s_vulkan->swapchain->extent.height
		),
		.format = vuk::Format::eR16G16Sfloat,
		.sample_count = vuk::Samples::e1,
		.level_count = 1,
		.layer_count = 1,
	});
	rg->attach_swapchain("swapchain", sys::s_vulkan->swapchain);
	rg->add_pass(vuk::Pass{
		.name = "primary rays",
		.resources = {
			"visibility/blank"_image >> vuk::eComputeWrite >> "visibility",
			"depth/blank"_image >> vuk::eComputeWrite >> "depth",
			"normal/blank"_image >> vuk::eComputeWrite >> "normal",
			"seed/blank"_image >> vuk::eComputeWrite >> "seed",
			"motion/blank"_image >> vuk::eComputeWrite >> "motion",
		},
		.execute = [this, &_camera](vuk::CommandBuffer& cmd) {
			cmd.bind_compute_pipeline("primary_ray")
				.bind_image(0, 0, "visibility/blank")
				.bind_image(0, 1, "depth/blank")
				.bind_image(0, 2, "normal/blank")
				.bind_image(0, 3, "seed/blank")
				.bind_image(0, 4, "motion/blank");

			struct Constants {
				mat4 view;
				mat4 prevView;
				uint frameCounter;
				float vFov;
			};
			auto* constants = cmd.map_scratch_buffer<Constants>(0, 5);
			*constants = Constants{
				.view = _camera.view(),
				.prevView = m_prevCamera.view(),
				.frameCounter = uint(sys::s_vulkan->context.get_frame_count()),
				.vFov = 60_deg,
			};

			cmd.dispatch_invocations(
				sys::s_vulkan->swapchain->extent.width,
				sys::s_vulkan->swapchain->extent.height
			);
		},
	});/*
	rg->add_pass(vuk::Pass{
		.name = "tonemapping",
		.resources = {
			"rt/out"_image >> vuk::eComputeSampled,
			"tonemapped/blank"_image >> vuk::eComputeWrite >> "tonemapped/out",
		},
		.execute = [](vuk::CommandBuffer& cmd) {
			cmd.bind_compute_pipeline("tonemap")
				.bind_image(0, 0, "rt/out").bind_sampler(0, 0, vuk::SamplerCreateInfo{
					.magFilter = vuk::Filter::eLinear,
					.minFilter = vuk::Filter::eLinear,
					.addressModeU = vuk::SamplerAddressMode::eClampToEdge,
					.addressModeV = vuk::SamplerAddressMode::eClampToEdge,
				})
				.bind_image(0, 1, "tonemapped/blank");

			cmd.dispatch_invocations(
				sys::s_vulkan->swapchain->extent.width,
				sys::s_vulkan->swapchain->extent.height
			);
		},
	});*/
	rg->add_pass(vuk::Pass{
		.name = "swapchain blit",
		.resources = {
			"normal"_image >> vuk::eTransferRead,
			"swapchain"_image >> vuk::eTransferWrite >> "swapchain/out",
		},
		.execute = [](vuk::CommandBuffer& cmd) {
			auto srcSize = cmd.get_resource_image_attachment("normal").value().extent.extent;
			auto dstSize = cmd.get_resource_image_attachment("swapchain").value().extent.extent;
			cmd.blit_image("normal", "swapchain", vuk::ImageBlit{
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
