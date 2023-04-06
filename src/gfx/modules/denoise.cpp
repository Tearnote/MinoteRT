#include "gfx/modules/denoise.hpp"

#include <vuk/CommandBuffer.hpp>
#include <vuk/RenderGraph.hpp>

#include "sys/vulkan.hpp"
#include "gfx/samplers.hpp"

namespace minote::gfx::modules {

auto denoiseBilateral(vuk::Future _color, vuk::Future _depth, vuk::Future _normal, Camera const& _camera, BilateralParams _params) -> vuk::Future {

	static auto compiled = false;
	if (!compiled) {
		compiled = true;

		auto denoiseBilateralPci = vuk::PipelineBaseCreateInfo();
		constexpr auto denoiseBilateralSource = std::to_array<uint>({
#include "spv/denoise/bilateral.comp.spv"
		});
		denoiseBilateralPci.add_static_spirv(denoiseBilateralSource.data(), denoiseBilateralSource.size(), "denoise/bilateral.comp");
		sys::s_vulkan->context.create_named_pipeline("denoise/bilateral", denoiseBilateralPci);
	}

	auto rg = std::make_shared<vuk::RenderGraph>("denoise/bilateral");
	rg->attach_in("color", std::move(_color));
	rg->attach_in("depth", std::move(_depth));
	rg->attach_in("normal", std::move(_normal));
	rg->attach_image("output/blank", vuk::ImageAttachment{
		.extent = vuk::Dimension3D::absolute(sys::s_vulkan->swapchain->extent), //TODO pending vuk bugfix
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
		.execute = [&_camera, _params](vuk::CommandBuffer& cmd) {
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
				.sigma = _params.sigma,
				.kSigma = _params.kSigma,
				.threshold = _params.threshold,
				.nearPlane = _camera.nearPlane,
				.frameCounter = uint(sys::s_vulkan->context.get_frame_count()),
			});

			auto size = cmd.get_resource_image_attachment("output/blank")->extent.extent;
			cmd.dispatch_invocations(size.width, size.height);
		},
	});

	return vuk::Future(std::move(rg), "output");

}

}
