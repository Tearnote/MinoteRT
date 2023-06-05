export module minote.modules.denoiser;

import <utility>;
import <memory>;
import <array>;
import <vuk/CommandBuffer.hpp>;
import <vuk/RenderGraph.hpp>;
import <vuk/Pipeline.hpp>;
import <vuk/Future.hpp>;
import minote.resource;
import minote.samplers;
import minote.vulkan;
import minote.camera;
import minote.math;

export class Denoiser:
	Vulkan
{
public:
	struct BilateralParams {
		float sigma;
		float kSigma;
		float threshold;

		static auto make_default() -> BilateralParams {
			return BilateralParams{
				.sigma = 5.0f,
				.kSigma = 2.0f,
				.threshold = 0.12f,
			};
		}
	};

	auto bilateral(vuk::Future color, vuk::Future depth, vuk::Future normal,
		Camera const& camera, BilateralParams params
	) -> vuk::Future {
		if (!compiledBilateral) {
			compiledBilateral = true;

			auto denoiseBilateralPci = vuk::PipelineBaseCreateInfo();
			constexpr auto denoiseBilateralSource = std::to_array<uint>({
#include "spv/denoise/bilateral.comp.spv"
			});
			denoiseBilateralPci.add_static_spirv(denoiseBilateralSource.data(), denoiseBilateralSource.size(), "denoise/bilateral.comp");
			Vulkan::serv->context.create_named_pipeline("denoise/bilateral", denoiseBilateralPci);
		}

		auto rg = std::make_shared<vuk::RenderGraph>("denoise/bilateral");
		rg->attach_in("color", std::move(color));
		rg->attach_in("depth", std::move(depth));
		rg->attach_in("normal", std::move(normal));
		rg->attach_image("output/blank", vuk::ImageAttachment{
			.extent = vuk::Dimension3D::absolute(Vulkan::serv->swapchain->extent), //TODO pending vuk bugfix
			.format = vuk::Format::eR8G8B8A8Unorm,
			.sample_count = vuk::Samples::e1,
			.level_count = 1,
			.layer_count = 1,
		});
		//	rg->inference_rule("output/blank", vuk::same_extent_as("input")); //TODO pending vuk bugfix

		rg->add_pass(vuk::Pass{
			.name = "denoise/bilateral",
			.resources = {
				"color"_image >> vuk::eComputeSampled,
				"depth"_image >> vuk::eComputeSampled,
				"normal"_image >> vuk::eComputeSampled,
				"output/blank"_image >> vuk::eComputeWrite >> "output",
			},
			.execute = [this, &camera, params](auto& cmd) {
				cmd.bind_compute_pipeline("denoise/bilateral")
					.bind_image(0, 0, "color").bind_sampler(0, 0, LinearClamp)
					.bind_image(0, 1, "depth").bind_sampler(0, 1, LinearClamp)
					.bind_image(0, 2, "normal").bind_sampler(0, 2, LinearClamp)
					.bind_image(0, 3, "output/blank");

				struct Constants {
					float sigma;
					float kSigma;
					float threshold;
					float nearPlane;
					uint frameCounter;
				};
				cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0, Constants{
					.sigma = params.sigma,
					.kSigma = params.kSigma,
					.threshold = params.threshold,
					.nearPlane = camera.nearPlane,
					.frameCounter = uint(Vulkan::serv->context.get_frame_count()),
				});

				auto size = cmd.get_resource_image_attachment("output/blank")->extent.extent;
				cmd.dispatch_invocations(size.width, size.height);
			},
			});

		return vuk::Future(std::move(rg), "output");
	}

private:
	static inline bool compiledBilateral = false;
};

export using BilateralParams = Denoiser::BilateralParams;
