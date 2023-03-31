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

auto tonemapReinhard(vuk::Future _input, float _exposure, float _hdrMax) -> vuk::Future {

	static auto compiled = false;
	if (!compiled) {
		compiled = true;

		auto tonemapReinhardPci = vuk::PipelineBaseCreateInfo();
		constexpr auto tonemapReinhardSource = std::to_array<uint>({
#include "spv/tonemap/reinhard.comp.spv"
		});
		tonemapReinhardPci.add_static_spirv(tonemapReinhardSource.data(), tonemapReinhardSource.size(), "tonemap/reinhard.comp");
		sys::s_vulkan->context.create_named_pipeline("tonemap/reinhard", tonemapReinhardPci);
	}

	auto rg = std::make_shared<vuk::RenderGraph>("tonemap/reinhard");
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
		.name = "tonemap/reinhard",
		.resources = {
			"input"_image >> vuk::eComputeSampled,
			"output/blank"_image >> vuk::eComputeWrite >> "output",
		},
		.execute = [_exposure, _hdrMax](vuk::CommandBuffer& cmd) {
			cmd.bind_compute_pipeline("tonemap/reinhard")
				.bind_image(0, 0, "input").bind_sampler(0, 0, NearestClamp)
				.bind_image(0, 1, "output/blank");

			struct Constants {
				float exposure;
				float hdrMax;
			};
			cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0, Constants{
				.exposure = _exposure,
				.hdrMax = _hdrMax,
			});

			auto size = cmd.get_resource_image_attachment("output/blank")->extent.extent;
			cmd.dispatch_invocations(size.width, size.height);
		},
	});

	return vuk::Future(std::move(rg), "output");

}

auto tonemapHable(vuk::Future _input, float _exposure) -> vuk::Future {

	static auto compiled = false;
	if (!compiled) {
		compiled = true;

		auto tonemapHablePci = vuk::PipelineBaseCreateInfo();
		constexpr auto tonemapHableSource = std::to_array<uint>({
#include "spv/tonemap/hable.comp.spv"
		});
		tonemapHablePci.add_static_spirv(tonemapHableSource.data(), tonemapHableSource.size(), "tonemap/hable.comp");
		sys::s_vulkan->context.create_named_pipeline("tonemap/hable", tonemapHablePci);
	}

	auto rg = std::make_shared<vuk::RenderGraph>("tonemap/hable");
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
		.name = "tonemap/hable",
		.resources = {
			"input"_image >> vuk::eComputeSampled,
			"output/blank"_image >> vuk::eComputeWrite >> "output",
		},
		.execute = [_exposure](vuk::CommandBuffer& cmd) {
			cmd.bind_compute_pipeline("tonemap/hable")
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

auto tonemapAces(vuk::Future _input, float _exposure) -> vuk::Future {

	static auto compiled = false;
	if (!compiled) {
		compiled = true;

		auto tonemapAcesPci = vuk::PipelineBaseCreateInfo();
		constexpr auto tonemapAcesSource = std::to_array<uint>({
#include "spv/tonemap/aces.comp.spv"
		});
		tonemapAcesPci.add_static_spirv(tonemapAcesSource.data(), tonemapAcesSource.size(), "tonemap/aces.comp");
		sys::s_vulkan->context.create_named_pipeline("tonemap/aces", tonemapAcesPci);
	}

	auto rg = std::make_shared<vuk::RenderGraph>("tonemap/aces");
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
		.name = "tonemap/aces",
		.resources = {
			"input"_image >> vuk::eComputeSampled,
			"output/blank"_image >> vuk::eComputeWrite >> "output",
		},
		.execute = [_exposure](vuk::CommandBuffer& cmd) {
			cmd.bind_compute_pipeline("tonemap/aces")
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

auto tonemapUchimura(vuk::Future _input, float _exposure, UchimuraParams const& _params) -> vuk::Future {

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
		.execute = [_exposure, _params](vuk::CommandBuffer& cmd) {
			cmd.bind_compute_pipeline("tonemap/uchimura")
				.bind_image(0, 0, "input").bind_sampler(0, 0, NearestClamp)
				.bind_image(0, 1, "output/blank");

			struct Constants {
				float exposure;
				float maxBrightness;
				float contrast;
				float linearStart;
				float linearLength;
				float blackTightness;
				float pedestal;
			};
			cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0, Constants{
				.exposure = _exposure,
				.maxBrightness = _params.maxBrightness,
				.contrast = _params.contrast,
				.linearStart = _params.linearStart,
				.linearLength = _params.linearLength,
				.blackTightness = _params.blackTightness,
				.pedestal = _params.pedestal,
			});

			auto size = cmd.get_resource_image_attachment("output/blank")->extent.extent;
			cmd.dispatch_invocations(size.width, size.height);
		},
	});

	return vuk::Future(std::move(rg), "output");

}

auto tonemapAMD(vuk::Future _input, float _exposure, AMDParams const& _params) -> vuk::Future {

	static auto compiled = false;
	if (!compiled) {
		compiled = true;

		auto tonemapAMDPci = vuk::PipelineBaseCreateInfo();
		constexpr auto tonemapAMDSource = std::to_array<uint>({
#include "spv/tonemap/amd.comp.spv"
		                                                           });
		tonemapAMDPci.add_static_spirv(tonemapAMDSource.data(), tonemapAMDSource.size(), "tonemap/amd.comp");
		sys::s_vulkan->context.create_named_pipeline("tonemap/amd", tonemapAMDPci);
	}

	auto rg = std::make_shared<vuk::RenderGraph>("tonemap/amd");
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
		.name = "tonemap/amd",
		.resources = {
			"input"_image >> vuk::eComputeSampled,
			"output/blank"_image >> vuk::eComputeWrite >> "output",
		},
		.execute = [_exposure, _params](vuk::CommandBuffer& cmd) {
			cmd.bind_compute_pipeline("tonemap/amd")
				.bind_image(0, 0, "input").bind_sampler(0, 0, NearestClamp)
				.bind_image(0, 1, "output/blank");

			struct Constants {
				float exposure;
				float hdrMax;
				float contrast;
				float shoulder;
				float midIn;
				float midOut;
			};
			cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0, Constants{
				.exposure = _exposure,
				.hdrMax = _params.hdrMax,
				.contrast = _params.contrast,
				.shoulder = _params.shoulder,
				.midIn = _params.midIn,
				.midOut = _params.midOut,
			});

			auto size = cmd.get_resource_image_attachment("output/blank")->extent.extent;
			cmd.dispatch_invocations(size.width, size.height);
		},
	});

	return vuk::Future(std::move(rg), "output");

}

}
