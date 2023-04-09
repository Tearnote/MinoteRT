#include "gfx/modules/sky.hpp"

#include <vuk/CommandBuffer.hpp>
#include <vuk/RenderGraph.hpp>
#include <vuk/Partials.hpp>

#include "sys/vulkan.hpp"
#include "gfx/samplers.hpp"

namespace minote::gfx::modules {

auto Atmosphere::Params::earth() -> Params {

	constexpr auto EarthRayleighScaleHeight = 8.0f;
	constexpr auto EarthMieScaleHeight = 1.2f;
	constexpr auto MieScattering = vec3{0.003996f, 0.003996f, 0.003996f};
	constexpr auto MieExtinction = vec3{0.004440f, 0.004440f, 0.004440f};

	return Params{
		.bottomRadius = 6360.0f,
		.topRadius = 6460.0f,
		.rayleighDensityExpScale = -1.0f / EarthRayleighScaleHeight,
		.rayleighScattering = {0.005802f, 0.013558f, 0.033100f},
		.mieDensityExpScale = -1.0f / EarthMieScaleHeight,
		.mieScattering = MieScattering,
		.mieExtinction = MieExtinction,
		.mieAbsorption = max(MieExtinction - MieScattering, vec3{0.0f, 0.0f, 0.0f}),
		.miePhaseG = 0.8f,
		.absorptionDensity0LayerWidth = 25.0f,
		.absorptionDensity0ConstantTerm = -2.0f / 3.0f,
		.absorptionDensity0LinearTerm = 1.0f / 15.0f,
		.absorptionDensity1ConstantTerm = 8.0f / 3.0f,
		.absorptionDensity1LinearTerm = -1.0f / 15.0f,
		.absorptionExtinction = {0.000650f, 0.001881f, 0.000085f},
		.groundAlbedo = {0.0f, 0.0f, 0.0f},
	};

}

Atmosphere::Atmosphere(vuk::Allocator& _allocator, Params const& _params) {

	static auto compiled = false;
	if (!compiled) {
		compiled = true;

		auto genTransmittancePci = vuk::PipelineBaseCreateInfo();
		constexpr auto genTransmittanceSource = std::to_array<uint>({
#include "spv/sky/genTransmittance.comp.spv"
		});
		genTransmittancePci.add_static_spirv(genTransmittanceSource.data(), genTransmittanceSource.size(), "sky/genTransmittance.comp");
		sys::s_vulkan->context.create_named_pipeline("sky/genTransmittance", genTransmittancePci);

		auto genMultiScatteringPci = vuk::PipelineBaseCreateInfo();
		constexpr auto genMultiScatteringSource = std::to_array<uint>({
#include "spv/sky/genMultiScattering.comp.spv"
		});
		genMultiScatteringPci.add_static_spirv(genMultiScatteringSource.data(), genMultiScatteringSource.size(), "sky/genMultiScattering.comp");
		sys::s_vulkan->context.create_named_pipeline("sky/genMultiScattering", genMultiScatteringPci);
	}

	auto rg = std::make_shared<vuk::RenderGraph>("atmosphere");
	rg->attach_image("transmittance/blank", vuk::ImageAttachment{
		.extent = vuk::Dimension3D::absolute(TransmittanceSize.x(), TransmittanceSize.y()),
		.format = TransmittanceFormat,
		.sample_count = vuk::Samples::e1,
		.level_count = 1,
		.layer_count = 1,
	});
	rg->attach_image("multiScattering/blank", vuk::ImageAttachment{
		.extent = vuk::Dimension3D::absolute(MultiScatteringSize.x(), MultiScatteringSize.y()),
		.format = MultiScatteringFormat,
		.sample_count = vuk::Samples::e1,
		.level_count = 1,
		.layer_count = 1,
	});
	auto paramsFut = vuk::create_buffer(_allocator, vuk::MemoryUsage::eGPUonly,
		vuk::DomainFlagBits::eGraphicsQueue,
		std::span(&_params, 1)).second;
	rg->attach_in("params", paramsFut);

	rg->add_pass(vuk::Pass{
		.name = "sky/genTransmittance",
		.resources = {
			"params"_buffer >> vuk::eComputeRead,
			"transmittance/blank"_image >> vuk::eComputeWrite >> "transmittance",
		},
		.execute = [](vuk::CommandBuffer& cmd) {
			cmd.bind_compute_pipeline("sky/genTransmittance");
			cmd.bind_buffer(0, 0, "params");
			cmd.bind_image(0, 1, "transmittance/blank");

			auto transmittance = *cmd.get_resource_image_attachment("transmittance/blank");
			auto transmittanceSize = transmittance.extent.extent;
			cmd.specialize_constants(0, transmittanceSize.width);
			cmd.specialize_constants(1, transmittanceSize.height);

			cmd.dispatch_invocations(transmittanceSize.width, transmittanceSize.height);
		},
	});

	rg->add_pass({
		.name = "sky/genMultiScattering",
		.resources = {
			"params"_buffer >> vuk::eComputeRead,
			"transmittance"_image >> vuk::eComputeSampled,
			"multiScattering/blank"_image >> vuk::eComputeWrite >> "multiScattering",
		},
		.execute = [](vuk::CommandBuffer& cmd) {

			cmd.bind_compute_pipeline("sky/genMultiScattering")
				.bind_buffer(0, 0, "params")
				.bind_image(0, 1, "transmittance").bind_sampler(0, 1, LinearClamp)
				.bind_image(0, 2, "multiScattering/blank");

			auto multiScattering = *cmd.get_resource_image_attachment("multiScattering/blank");
			auto multiScatteringSize = multiScattering.extent.extent;
			cmd.specialize_constants(0, multiScatteringSize.width);
			cmd.specialize_constants(1, multiScatteringSize.height);

			cmd.dispatch_invocations(multiScatteringSize.width, multiScatteringSize.height);

		},
	});

	params = vuk::Future(rg, "params");
	transmittance = vuk::Future(rg, "transmittance");
	multiScattering = vuk::Future(rg, "multiScattering");

}

auto Sky::createView(Atmosphere const& _atmo, vec3 _probePos) -> Texture2D<vec3> {
	static auto compiled = false;
	if (!compiled) {
		compiled = true;

		auto genViewPci = vuk::PipelineBaseCreateInfo();
		constexpr auto genViewSource = std::to_array<uint>({
#include "spv/sky/genView.comp.spv"
		});
		genViewPci.add_static_spirv(genViewSource.data(), genViewSource.size(), "sky/genView.comp");
		sys::s_vulkan->context.create_named_pipeline("sky/genView", genViewPci);
	}

	auto rg = std::make_shared<vuk::RenderGraph>("sky");
	rg->attach_image("view/blank", vuk::ImageAttachment{
		.extent = vuk::Dimension3D::absolute(ViewSize.x(), ViewSize.y()),
		.format = ViewFormat,
		.sample_count = vuk::Samples::e1,
		.level_count = 1,
		.layer_count = 1,
	});
	rg->attach_in("params", _atmo.params);
	rg->attach_in("transmittance", _atmo.transmittance);
	rg->attach_in("multiScattering", _atmo.multiScattering);

	rg->add_pass(vuk::Pass{
		.name = "sky/genView",
		.resources = {
			"params"_buffer >> vuk::eComputeRead,
			"transmittance"_image >> vuk::eComputeSampled,
			"multiScattering"_image >> vuk::eComputeSampled,
			"view/blank"_image >> vuk::eComputeWrite >> "view",
		},
		.execute = [this, _probePos](vuk::CommandBuffer& cmd) {

			cmd.bind_compute_pipeline("sky/genView")
				.bind_buffer(0, 0, "params")
				.bind_image(0, 1, "transmittance").bind_sampler(0, 1, LinearClamp)
				.bind_image(0, 2, "multiScattering").bind_sampler(0, 2, LinearClamp)
				.bind_image(0, 3, "view/blank");

			struct Constants {
				vec3 probePos;
				float _pad0;
				vec3 sunDirection;
				float _pad1;
				vec3 sunIlluminance;
			};
			cmd.push_constants(vuk::ShaderStageFlagBits::eCompute, 0, Constants{
				.probePos = _probePos,
				.sunDirection = sunDirection,
				.sunIlluminance = sunIlluminance,
			});

			auto view = *cmd.get_resource_image_attachment("view/blank");
			auto viewSize = view.extent.extent;
			cmd.specialize_constants(0, viewSize.width);
			cmd.specialize_constants(1, viewSize.height);

			cmd.dispatch_invocations(viewSize.width, viewSize.height);

		},
	});

	return vuk::Future(rg, "view");
}

}
