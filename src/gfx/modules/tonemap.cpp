#include "gfx/modules/tonemap.hpp"

#include <utility>
#include <memory>
#include <array>

#include <vuk/CommandBuffer.hpp>
#include <vuk/RenderGraph.hpp>

#include "types.hpp"
#include "sys/vulkan.hpp"
#include "gfx/samplers.hpp"

namespace minote::gfx::modules {

auto tonemap(vuk::Future _input, TonemapMode _mode, float _exposure) -> vuk::Future {

	static auto compiled = false;
	if (!compiled) {
		compiled = true;

		auto tonemapPci = vuk::PipelineBaseCreateInfo();
		constexpr auto tonemapSource = std::to_array<uint>({
#include "spv/tonemap.comp.spv"
		});
		tonemapPci.add_static_spirv(tonemapSource.data(), tonemapSource.size(), "tonemap.comp");
		sys::s_vulkan->context.create_named_pipeline("tonemap", tonemapPci);
	}

	auto rg = std::make_shared<vuk::RenderGraph>("tonemap");
	rg->attach_in("input", std::move(_input));
	rg->attach_image("output/blank", vuk::ImageAttachment{
		.extent = vuk::Dimension3D::absolute(sys::s_vulkan->swapchain->extent), //TODO pending vuk bugfix
		.format = vuk::Format::eR8G8B8A8Unorm,
		.sample_count = vuk::Samples::e1,
		.level_count = 1,
		.layer_count = 1,
	});
//	rg->inference_rule("output/blank", vuk::same_extent_as("input")); //TODO pending vuk bugfix

	rg->add_pass(vuk::Pass{
		.name = "tonemapping",
		.resources = {
			"input"_image >> vuk::eComputeSampled,
			"output/blank"_image >> vuk::eComputeWrite >> "output",
		},
		.execute = [_mode, _exposure](vuk::CommandBuffer& cmd) {
			cmd.bind_compute_pipeline("tonemap")
				.bind_image(0, 0, "input").bind_sampler(0, 0, NearestClamp)
				.bind_image(0, 1, "output/blank");

			struct Constants {
				uint uchimura;
				float exposure;
			};
			cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0, Constants{
				.uchimura = _mode == TonemapMode::Uchimura? 1u : 0u,
				.exposure = _exposure,
			});

			auto size = cmd.get_resource_image_attachment("output/blank")->extent.extent;
			cmd.dispatch_invocations(size.width, size.height);
		},
	});

	return vuk::Future(std::move(rg), "output");

}

}
