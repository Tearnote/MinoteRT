export module minote.modules.pathtracer;

import <utility>;
import <memory>;
import <array>;
import <vuk/CommandBuffer.hpp>;
import <vuk/RenderGraph.hpp>;
import <vuk/Pipeline.hpp>;
import <vuk/Future.hpp>;
import <vuk/Image.hpp>;
import minote.modules.sky;
import minote.resource;
import minote.samplers;
import minote.camera;
import minote.vulkan;
import minote.math;

export class Pathtracer:
	Vulkan
{
public:
	struct GBuffer {
		vuk::Future visibility;
		vuk::Future depth;
		vuk::Future normal;
		vuk::Future motion;
	};

	auto primaryRays(uvec2 size, Camera const& camera, Camera const& prevCamera) -> GBuffer {
		if (!compiledPrimary) {
			compiledPrimary = true;

			auto primaryRayPci = vuk::PipelineBaseCreateInfo();
			constexpr auto primaryRaySource = std::to_array<uint>({
	#include "spv/primaryRay.comp.spv"
			});
			primaryRayPci.add_static_spirv(primaryRaySource.data(), primaryRaySource.size(), "primaryRay.comp");
			Vulkan::serv->context.create_named_pipeline("primary_ray", primaryRayPci);
		}

		auto rg = std::make_shared<vuk::RenderGraph>("primary_rays");
		rg->attach_image("visibility/blank", vuk::ImageAttachment{
			.extent = vuk::Dimension3D::absolute(size.x(), size.y(), 1u),
			.format = vuk::Format::eR32Uint,
			.sample_count = vuk::Samples::e1,
			.level_count = 1,
			.layer_count = 1,
		});
		rg->attach_image("depth/blank", vuk::ImageAttachment{
			.extent = vuk::Dimension3D::absolute(size.x(), size.y(), 1u),
			.format = vuk::Format::eR16Sfloat,
			.sample_count = vuk::Samples::e1,
			.level_count = 1,
			.layer_count = 1,
		});
		rg->attach_image("normal/blank", vuk::ImageAttachment{
			.extent = vuk::Dimension3D::absolute(size.x(), size.y(), 1u),
			.format = vuk::Format::eR16G16B16A16Sfloat, // w unused
			.sample_count = vuk::Samples::e1,
			.level_count = 1,
			.layer_count = 1,
		});
		rg->attach_image("motion/blank", vuk::ImageAttachment{
			.extent = vuk::Dimension3D::absolute(size.x(), size.y(), 1u),
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
			.execute = [this, size, &camera, &prevCamera](auto& cmd) {
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
				auto* constants = cmd.map_scratch_buffer<Constants>(0, 4);
				mat4 view = camera.view();
				mat4 projection = camera.projection();
				*constants = Constants{
					.view = view,
					.projection = projection,
					.invView = inverse(view),
					.invProjection = inverse(projection),
					.prevView = prevCamera.view(),
					.frameCounter = uint(Vulkan::serv->context.get_frame_count()),
				};

				cmd.dispatch_invocations(size.x(), size.y());
			},
		});

		return GBuffer{
			.visibility = vuk::Future(rg, "visibility"),
			.depth = vuk::Future(rg, "depth"),
			.normal = vuk::Future(rg, "normal"),
			.motion = vuk::Future(rg, "motion"),
		};
	}

	auto secondaryRays(GBuffer gbuffer, Camera const& camera, Atmosphere const& atmo,
		Texture2D<vec3> skyView, vuk::Texture& blueNoise
	) -> vuk::Future {
		if (!compiledSecondary) {
			compiledSecondary = true;

			auto secondaryRaysPci = vuk::PipelineBaseCreateInfo();
			constexpr auto secondaryRaysSource = std::to_array<uint>({
#include "spv/secondaryRays.comp.spv"
			});
			secondaryRaysPci.add_static_spirv(secondaryRaysSource.data(), secondaryRaysSource.size(), "secondaryRays.comp");
			Vulkan::serv->context.create_named_pipeline("secondary_rays", secondaryRaysPci);
		}

		auto rg = std::make_shared<vuk::RenderGraph>("secondary_rays");
		rg->attach_in("visibility", std::move(gbuffer.visibility));
		rg->attach_in("depth", std::move(gbuffer.depth));
		rg->attach_in("normal", std::move(gbuffer.normal));
		rg->attach_in("transmittance", atmo.transmittance);
		rg->attach_in("skyView", std::move(skyView));
		rg->attach_in("atmoParams", atmo.params);
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
				"skyView"_image >> vuk::eComputeSampled,
				"transmittance"_image >> vuk::eComputeSampled,
				"atmoParams"_buffer >> vuk::eComputeRead,
				"color/blank"_image >> vuk::eComputeWrite >> "color",
			},
			.execute = [this, &camera, &blueNoise](auto& cmd) {
				cmd.bind_compute_pipeline("secondary_rays")
					.bind_image(0, 0, "visibility").bind_sampler(0, 0, NearestClamp)
					.bind_image(0, 1, "depth").bind_sampler(0, 1, NearestClamp)
					.bind_image(0, 2, "normal").bind_sampler(0, 2, NearestClamp)
					.bind_image(0, 3, *blueNoise.view, vuk::ImageLayout::eShaderReadOnlyOptimal).bind_sampler(0, 3, NearestClamp)
					.bind_image(0, 4, "transmittance").bind_sampler(0, 4, LinearClamp)
					.bind_image(0, 5, "skyView").bind_sampler(0, 5, LinearRepeat)
					.bind_buffer(0, 6, "atmoParams")
					.bind_image(0, 7, "color/blank");

				auto colorSize = cmd.get_resource_image_attachment("color/blank").value().extent.extent;
				struct Constants {
					mat4 view;
					mat4 projection;
					mat4 invView;
					mat4 invProjection;
					vec3 cameraPos;
					uint frameCounter;
				};
				auto* constants = cmd.map_scratch_buffer<Constants>(0, 8);
				mat4 view = camera.view();
				mat4 projection = camera.projection();
				*constants = Constants{
					.view = view,
					.projection = projection,
					.invView = inverse(view),
					.invProjection = inverse(projection),
					.cameraPos = camera.position,
					.frameCounter = uint(Vulkan::serv->context.get_frame_count()),
				};

				cmd.dispatch_invocations(colorSize.width, colorSize.height);
			},
		});

		return vuk::Future(std::move(rg), "color");
	}

private:
	static inline bool compiledPrimary = false;
	static inline bool compiledSecondary = false;
};
