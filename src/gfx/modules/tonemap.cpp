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

auto tonemapLinear(vuk::Future _input, float _exposure) -> vuk::Future {

	static auto compiled = false;
	if (!compiled) {
		compiled = true;

		auto tonemapLinearPci = vuk::PipelineBaseCreateInfo();
		constexpr auto tonemapLinearSource = std::to_array<uint>({
#include "spv/tonemap/linear.comp.spv"
		                                                   });
		tonemapLinearPci.add_static_spirv(tonemapLinearSource.data(), tonemapLinearSource.size(), "tonemap/linear.comp");
		sys::s_vulkan->context.create_named_pipeline("tonemap/linear", tonemapLinearPci);
	}

	auto rg = std::make_shared<vuk::RenderGraph>("tonemap/linear");
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
		.name = "tonemap/linear",
		.resources = {
			"input"_image >> vuk::eComputeSampled,
			"output/blank"_image >> vuk::eComputeWrite >> "output",
		},
		.execute = [_exposure](vuk::CommandBuffer& cmd) {
			cmd.bind_compute_pipeline("tonemap/linear")
				.bind_image(0, 0, "input").bind_sampler(0, 0, NearestClamp)
				.bind_image(0, 1, "output/blank");

			struct Constants {
				float exposure;
			};
			cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0, Constants{
				.exposure = _exposure,
			});

			auto size = cmd.get_resource_image_attachment("output/blank")->extent.extent;
			cmd.dispatch_invocations(size.width, size.height);
		},
	});

	return vuk::Future(std::move(rg), "output");

}

auto tonemapUchimura(vuk::Future _input, float _exposure) -> vuk::Future {

	static auto compiled = false;
	if (!compiled) {
		compiled = true;

		auto tonemapUchimuraPci = vuk::PipelineBaseCreateInfo();
		constexpr auto tonemapUchimuraSource = std::to_array<uint>({
#include "spv/tonemap/uchimura.comp.spv"
		});
		tonemapUchimuraPci.add_static_spirv(tonemapUchimuraSource.data(), tonemapUchimuraSource.size(), "tonemap/uchimura.comp");
		sys::s_vulkan->context.create_named_pipeline("tonemap/uchimura", tonemapUchimuraPci);
	}

	auto rg = std::make_shared<vuk::RenderGraph>("tonemap/uchimura");
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
		.name = "tonemap/uchimura",
		.resources = {
			"input"_image >> vuk::eComputeSampled,
			"output/blank"_image >> vuk::eComputeWrite >> "output",
		},
		.execute = [_exposure](vuk::CommandBuffer& cmd) {
			cmd.bind_compute_pipeline("tonemap/uchimura")
				.bind_image(0, 0, "input").bind_sampler(0, 0, NearestClamp)
				.bind_image(0, 1, "output/blank");

			struct Constants {
				float exposure;
			};
			cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0, Constants{
				.exposure = _exposure,
			});

			auto size = cmd.get_resource_image_attachment("output/blank")->extent.extent;
			cmd.dispatch_invocations(size.width, size.height);
		},
	});

	return vuk::Future(std::move(rg), "output");

}

}
