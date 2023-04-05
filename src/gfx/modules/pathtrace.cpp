#include "gfx/modules/pathtrace.hpp"

#include <utility>
#include <memory>
#include <array>

#include <vuk/CommandBuffer.hpp>
#include <vuk/RenderGraph.hpp>

#include "types.hpp"
#include "sys/vulkan.hpp"
#include "gfx/samplers.hpp"

namespace minote::gfx::modules {

using namespace math_literals;

auto primaryRays(uvec2 _size, Camera const& _camera, Camera const& _prevCamera) -> GBuffer {

	static auto compiled = false;
	if (!compiled) {
		compiled = true;

		auto primaryRayPci = vuk::PipelineBaseCreateInfo();
		constexpr auto primaryRaySource = std::to_array<uint>({
#include "spv/primaryRay.comp.spv"
		});
		primaryRayPci.add_static_spirv(primaryRaySource.data(), primaryRaySource.size(), "primaryRay.comp");
		sys::s_vulkan->context.create_named_pipeline("primary_ray", primaryRayPci);
	}

	auto rg = std::make_shared<vuk::RenderGraph>("primary_rays");
	rg->attach_image("visibility/blank", vuk::ImageAttachment{
		.extent = vuk::Dimension3D::absolute(_size.x(), _size.y(), 1u),
		.format = vuk::Format::eR32Uint,
		.sample_count = vuk::Samples::e1,
		.level_count = 1,
		.layer_count = 1,
	});
	rg->attach_image("depth/blank", vuk::ImageAttachment{
		.extent = vuk::Dimension3D::absolute(_size.x(), _size.y(), 1u),
		.format = vuk::Format::eR16Sfloat,
		.sample_count = vuk::Samples::e1,
		.level_count = 1,
		.layer_count = 1,
	});
	rg->attach_image("normal/blank", vuk::ImageAttachment{
		.extent = vuk::Dimension3D::absolute(_size.x(), _size.y(), 1u),
		.format = vuk::Format::eR16G16B16A16Sfloat, // w unused
		.sample_count = vuk::Samples::e1,
		.level_count = 1,
		.layer_count = 1,
	});
	rg->attach_image("motion/blank", vuk::ImageAttachment{
		.extent = vuk::Dimension3D::absolute(_size.x(), _size.y(), 1u),
		.format = vuk::Format::eR16G16Sfloat,
		.sample_count = vuk::Samples::e1,
		.level_count = 1,
		.layer_count = 1,
	});

	rg->add_pass(vuk::Pass{
		.name = "primary rays",
		.resources = {
			"visibility/blank"_image >> vuk::eComputeWrite >> "visibility",
			"depth/blank"_image >> vuk::eComputeWrite >> "depth",
			"normal/blank"_image >> vuk::eComputeWrite >> "normal",
			"motion/blank"_image >> vuk::eComputeWrite >> "motion",
		},
		.execute = [_size, &_camera, &_prevCamera](vuk::CommandBuffer& cmd) {
			cmd.bind_compute_pipeline("primary_ray")
				.bind_image(0, 0, "visibility/blank")
				.bind_image(0, 1, "depth/blank")
				.bind_image(0, 2, "normal/blank")
				.bind_image(0, 3, "motion/blank");

			struct Constants {
				mat4 view;
				mat4 projection;
				mat4 invView;
				mat4 invProjection;
				mat4 prevView;
				uint frameCounter;
			};
			auto colorSize = cmd.get_resource_image_attachment("visibility/blank").value().extent.extent;
			auto* constants = cmd.map_scratch_buffer<Constants>(0, 4);
			mat4 view = _camera.view();
			mat4 projection = _camera.projection();
			*constants = Constants{
				.view = view,
				.projection = projection,
				.invView = inverse(view),
				.invProjection = inverse(projection),
				.prevView = _prevCamera.view(),
				.frameCounter = uint(sys::s_vulkan->context.get_frame_count()),
			};

			cmd.dispatch_invocations(_size.x(), _size.y());
		},
	});

	return GBuffer{
		.visibility = vuk::Future(rg, "visibility"),
		.depth = vuk::Future(rg, "depth"),
		.normal = vuk::Future(rg, "normal"),
		.motion = vuk::Future(rg, "motion"),
	};

}

auto secondaryRays(GBuffer _gbuffer, Camera const& _camera, vuk::Texture& _blueNoise) -> vuk::Future {

	static auto compiled = false;
	if (!compiled) {
		compiled = true;

		auto secondaryRaysPci = vuk::PipelineBaseCreateInfo();
		constexpr auto secondaryRaysSource = std::to_array<uint>({
#include "spv/secondaryRays.comp.spv"
		});
		secondaryRaysPci.add_static_spirv(secondaryRaysSource.data(), secondaryRaysSource.size(), "secondaryRays.comp");
		sys::s_vulkan->context.create_named_pipeline("secondary_rays", secondaryRaysPci);
	}

	auto rg = std::make_shared<vuk::RenderGraph>("secondary_rays");
	rg->attach_in("visibility", std::move(_gbuffer.visibility));
	rg->attach_in("depth", std::move(_gbuffer.depth));
	rg->attach_in("normal", std::move(_gbuffer.normal));
	rg->attach_image("color/blank", vuk::ImageAttachment{
		.format = vuk::Format::eR16G16B16A16Sfloat,
		.sample_count = vuk::Samples::e1,
		.level_count = 1,
		.layer_count = 1,
	});
	rg->inference_rule("color/blank", vuk::same_extent_as("visibility"));

	rg->add_pass(vuk::Pass{
		.name = "secondary rays",
		.resources = {
			"visibility"_image >> vuk::eComputeSampled,
			"depth"_image >> vuk::eComputeSampled,
			"normal"_image >> vuk::eComputeSampled,
			"color/blank"_image >> vuk::eComputeWrite >> "color",
		},
		.execute = [&_camera, &_blueNoise](vuk::CommandBuffer& cmd) {
			cmd.bind_compute_pipeline("secondary_rays")
				.bind_image(0, 0, "visibility").bind_sampler(0, 0, NearestClamp)
				.bind_image(0, 1, "depth").bind_sampler(0, 1, NearestClamp)
				.bind_image(0, 2, "normal").bind_sampler(0, 2, NearestClamp)
				.bind_image(0, 3, *_blueNoise.view, vuk::ImageLayout::eShaderReadOnlyOptimal).bind_sampler(0, 3, NearestClamp)
				.bind_image(0, 4, "color/blank");

			auto colorSize = cmd.get_resource_image_attachment("color/blank").value().extent.extent;
			struct Constants {
				mat4 view;
				mat4 projection;
				mat4 invView;
				mat4 invProjection;
				uint frameCounter;
			};
			auto* constants = cmd.map_scratch_buffer<Constants>(0, 5);
			mat4 view = _camera.view();
			mat4 projection = _camera.projection();
			*constants = Constants{
				.view = view,
				.projection = projection,
				.invView = inverse(view),
				.invProjection = inverse(projection),
				.frameCounter = uint(sys::s_vulkan->context.get_frame_count()),
			};

			cmd.dispatch_invocations(colorSize.width, colorSize.height);
		},
	});

	return vuk::Future(std::move(rg), "color");

}

}
