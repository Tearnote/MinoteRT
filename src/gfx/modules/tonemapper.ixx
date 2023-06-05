export module minote.modules.tonemapper;

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
import minote.math;

export class Tonemapper:
	Vulkan
{
public:
	struct UchimuraParams {
		float maxBrightness;
		float contrast;
		float linearStart;
		float linearLength;
		float blackTightness;
		float pedestal;

		static auto make_default() -> UchimuraParams {
			return UchimuraParams{
				.maxBrightness = 1.0f,
				.contrast = 1.0f,
				.linearStart = 0.22f,
				.linearLength = 0.4f,
				.blackTightness = 1.33f,
				.pedestal = 0.0f,
			};
		}
	};

	struct AMDParams {
		float hdrMax;
		float contrast;
		float shoulder;
		float midIn;
		float midOut;

		static auto make_default() -> AMDParams {
			return AMDParams{
				.hdrMax = 16.0f,
				.contrast = 2.0f,
				.shoulder = 1.0f,
				.midIn = 0.18f,
				.midOut = 0.18f,
			};
		}
	};

	auto linear(vuk::Future input, float exposure) -> vuk::Future {
		if (!compiledLinear) {
			compiledLinear = true;

			auto tonemapLinearPci = vuk::PipelineBaseCreateInfo();
			constexpr auto tonemapLinearSource = std::to_array<uint>({
#include "spv/tonemap/linear.comp.spv"
			});
			tonemapLinearPci.add_static_spirv(tonemapLinearSource.data(), tonemapLinearSource.size(), "tonemap/linear.comp");
			Vulkan::serv->context.create_named_pipeline("tonemap/linear", tonemapLinearPci);
		}

		auto rg = std::make_shared<vuk::RenderGraph>("tonemap/linear");
		rg->attach_in("input", std::move(input));
		rg->attach_image("output/blank", vuk::ImageAttachment{
			.extent = vuk::Dimension3D::absolute(Vulkan::serv->swapchain->extent), //TODO pending vuk bugfix
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
			.execute = [exposure](auto& cmd) {
				cmd.bind_compute_pipeline("tonemap/linear")
					.bind_image(0, 0, "input").bind_sampler(0, 0, NearestClamp)
					.bind_image(0, 1, "output/blank");

				struct Constants {
					float exposure;
				};
				cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0, Constants{
					.exposure = exposure,
				});

				auto size = cmd.get_resource_image_attachment("output/blank")->extent.extent;
				cmd.dispatch_invocations(size.width, size.height);
			},
		});

		return vuk::Future(std::move(rg), "output");
	}

	auto reinhard(vuk::Future input, float exposure, float hdrMax) -> vuk::Future {
		if (!compiledReinhard) {
			compiledReinhard = true;

			auto tonemapReinhardPci = vuk::PipelineBaseCreateInfo();
			constexpr auto tonemapReinhardSource = std::to_array<uint>({
#include "spv/tonemap/reinhard.comp.spv"
			});
			tonemapReinhardPci.add_static_spirv(tonemapReinhardSource.data(), tonemapReinhardSource.size(), "tonemap/reinhard.comp");
			Vulkan::serv->context.create_named_pipeline("tonemap/reinhard", tonemapReinhardPci);
		}

		auto rg = std::make_shared<vuk::RenderGraph>("tonemap/reinhard");
		rg->attach_in("input", std::move(input));
		rg->attach_image("output/blank", vuk::ImageAttachment{
			.extent = vuk::Dimension3D::absolute(Vulkan::serv->swapchain->extent), //TODO pending vuk bugfix
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
			.execute = [exposure, hdrMax](auto& cmd) {
				cmd.bind_compute_pipeline("tonemap/reinhard")
					.bind_image(0, 0, "input").bind_sampler(0, 0, NearestClamp)
					.bind_image(0, 1, "output/blank");

				struct Constants {
					float exposure;
					float hdrMax;
				};
				cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0, Constants{
					.exposure = exposure,
					.hdrMax = hdrMax,
				});

				auto size = cmd.get_resource_image_attachment("output/blank")->extent.extent;
				cmd.dispatch_invocations(size.width, size.height);
			},
		});

		return vuk::Future(std::move(rg), "output");
	}

	auto hable(vuk::Future input, float exposure) -> vuk::Future {
		if (!compiledHable) {
			compiledHable = true;

			auto tonemapHablePci = vuk::PipelineBaseCreateInfo();
			constexpr auto tonemapHableSource = std::to_array<uint>({
	#include "spv/tonemap/hable.comp.spv"
				});
			tonemapHablePci.add_static_spirv(tonemapHableSource.data(), tonemapHableSource.size(), "tonemap/hable.comp");
			Vulkan::serv->context.create_named_pipeline("tonemap/hable", tonemapHablePci);
		}

		auto rg = std::make_shared<vuk::RenderGraph>("tonemap/hable");
		rg->attach_in("input", std::move(input));
		rg->attach_image("output/blank", vuk::ImageAttachment{
			.extent = vuk::Dimension3D::absolute(Vulkan::serv->swapchain->extent), //TODO pending vuk bugfix
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
			.execute = [exposure](auto& cmd) {
				cmd.bind_compute_pipeline("tonemap/hable")
					.bind_image(0, 0, "input").bind_sampler(0, 0, NearestClamp)
					.bind_image(0, 1, "output/blank");

				struct Constants {
					float exposure;
				};
				cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0, Constants{
					.exposure = exposure,
				});

				auto size = cmd.get_resource_image_attachment("output/blank")->extent.extent;
				cmd.dispatch_invocations(size.width, size.height);
			},
		});

		return vuk::Future(std::move(rg), "output");
	}

	auto aces(vuk::Future input, float exposure) -> vuk::Future {
		if (!compiledAces) {
			compiledAces = true;

			auto tonemapAcesPci = vuk::PipelineBaseCreateInfo();
			constexpr auto tonemapAcesSource = std::to_array<uint>({
#include "spv/tonemap/aces.comp.spv"
			});
			tonemapAcesPci.add_static_spirv(tonemapAcesSource.data(), tonemapAcesSource.size(), "tonemap/aces.comp");
			Vulkan::serv->context.create_named_pipeline("tonemap/aces", tonemapAcesPci);
		}

		auto rg = std::make_shared<vuk::RenderGraph>("tonemap/aces");
		rg->attach_in("input", std::move(input));
		rg->attach_image("output/blank", vuk::ImageAttachment{
			.extent = vuk::Dimension3D::absolute(Vulkan::serv->swapchain->extent), //TODO pending vuk bugfix
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
			.execute = [exposure](auto& cmd) {
				cmd.bind_compute_pipeline("tonemap/aces")
					.bind_image(0, 0, "input").bind_sampler(0, 0, NearestClamp)
					.bind_image(0, 1, "output/blank");

				struct Constants {
					float exposure;
				};
				cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0, Constants{
					.exposure = exposure,
				});

				auto size = cmd.get_resource_image_attachment("output/blank")->extent.extent;
				cmd.dispatch_invocations(size.width, size.height);
			},
		});

		return vuk::Future(std::move(rg), "output");
	}

	auto uchimura(vuk::Future input, float exposure, UchimuraParams const& params) -> vuk::Future {
		if (!compiledUchimura) {
			compiledUchimura = true;

			auto tonemapUchimuraPci = vuk::PipelineBaseCreateInfo();
			constexpr auto tonemapUchimuraSource = std::to_array<uint>({
#include "spv/tonemap/uchimura.comp.spv"
			});
			tonemapUchimuraPci.add_static_spirv(tonemapUchimuraSource.data(), tonemapUchimuraSource.size(), "tonemap/uchimura.comp");
			Vulkan::serv->context.create_named_pipeline("tonemap/uchimura", tonemapUchimuraPci);
		}

		auto rg = std::make_shared<vuk::RenderGraph>("tonemap/uchimura");
		rg->attach_in("input", std::move(input));
		rg->attach_image("output/blank", vuk::ImageAttachment{
			.extent = vuk::Dimension3D::absolute(Vulkan::serv->swapchain->extent), //TODO pending vuk bugfix
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
			.execute = [exposure, params](auto& cmd) {
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
					.exposure = exposure,
					.maxBrightness = params.maxBrightness,
					.contrast = params.contrast,
					.linearStart = params.linearStart,
					.linearLength = params.linearLength,
					.blackTightness = params.blackTightness,
					.pedestal = params.pedestal,
				});

				auto size = cmd.get_resource_image_attachment("output/blank")->extent.extent;
				cmd.dispatch_invocations(size.width, size.height);
			},
		});

		return vuk::Future(std::move(rg), "output");
	}

	auto amd(vuk::Future input, float exposure, AMDParams const& params) -> vuk::Future {
		if (!compiledAmd) {
			compiledAmd = true;

			auto tonemapAMDPci = vuk::PipelineBaseCreateInfo();
			constexpr auto tonemapAMDSource = std::to_array<uint>({
	#include "spv/tonemap/amd.comp.spv"
				});
			tonemapAMDPci.add_static_spirv(tonemapAMDSource.data(), tonemapAMDSource.size(), "tonemap/amd.comp");
			Vulkan::serv->context.create_named_pipeline("tonemap/amd", tonemapAMDPci);
		}

		auto rg = std::make_shared<vuk::RenderGraph>("tonemap/amd");
		rg->attach_in("input", std::move(input));
		rg->attach_image("output/blank", vuk::ImageAttachment{
			.extent = vuk::Dimension3D::absolute(Vulkan::serv->swapchain->extent), //TODO pending vuk bugfix
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
			.execute = [exposure, params](auto& cmd) {
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
					.exposure = exposure,
					.hdrMax = params.hdrMax,
					.contrast = params.contrast,
					.shoulder = params.shoulder,
					.midIn = params.midIn,
					.midOut = params.midOut,
				});

				auto size = cmd.get_resource_image_attachment("output/blank")->extent.extent;
				cmd.dispatch_invocations(size.width, size.height);
			},
		});

		return vuk::Future(std::move(rg), "output");
	}

private:
	static inline bool compiledLinear = false;
	static inline bool compiledReinhard = false;
	static inline bool compiledHable = false;
	static inline bool compiledAces = false;
	static inline bool compiledUchimura = false;
	static inline bool compiledAmd = false;
};

export using UchimuraParams = Tonemapper::UchimuraParams;
export using AMDParams = Tonemapper::AMDParams;
