export module minote.modules.sky;

import <memory>;
import <array>;
import <span>;
import <vuk/CommandBuffer.hpp>;
import <vuk/RenderGraph.hpp>;
import <vuk/Pipeline.hpp>;
import <vuk/Partials.hpp>;
import <vuk/Future.hpp>;
import minote.resource;
import minote.samplers;
import minote.vulkan;
import minote.math;

// Precalculated representation of a planet's atmosphere. Once created, it can
// be used repeatedly to sample the sky at any elevation and sun position
export class Atmosphere:
	Vulkan
{
public:
	constexpr static auto TransmittanceFormat = vuk::Format::eR16G16B16A16Sfloat;
	constexpr static auto TransmittanceSize = uvec2{ 256u, 64u };

	constexpr static auto MultiScatteringFormat = vuk::Format::eR16G16B16A16Sfloat;
	constexpr static auto MultiScatteringSize = uvec2{ 32u, 32u };

	struct Params {
		float bottomRadius; // Radius of the planet (center to ground)
		float topRadius; // Maximum considered atmosphere height (center to atmosphere top)

		float rayleighDensityExpScale; // Rayleigh scattering exponential distribution scale in the atmosphere
		float _pad0;
		vec3 rayleighScattering; // Rayleigh scattering coefficients

		float mieDensityExpScale; // Mie scattering exponential distribution scale in the atmosphere
		vec3 mieScattering; // Mie scattering coefficients
		float _pad1;
		vec3 mieExtinction; // Mie extinction coefficients
		float _pad2;
		vec3 mieAbsorption; // Mie absorption coefficients
		float miePhaseG; // Mie phase function excentricity

		// Another medium type in the atmosphere
		float absorptionDensity0LayerWidth;
		float absorptionDensity0ConstantTerm;
		float absorptionDensity0LinearTerm;
		float absorptionDensity1ConstantTerm;
		float absorptionDensity1LinearTerm;
		float _pad3;
		float _pad4;
		float _pad5;
		vec3 absorptionExtinction; // This other medium only absorb light, e.g. useful to represent ozone in the earth atmosphere
		float _pad6;

		vec3 groundAlbedo;

		// Return params that model Earth's atmosphere
		static auto earth() -> Params {
			constexpr auto EarthRayleighScaleHeight = 8.0f;
			constexpr auto EarthMieScaleHeight = 1.2f;
			constexpr auto MieScattering = vec3{ 0.003996f, 0.003996f, 0.003996f };
			constexpr auto MieExtinction = vec3{ 0.004440f, 0.004440f, 0.004440f };

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
	};

	Texture2D<vec4> transmittance;
	Texture2D<vec4> multiScattering;
	Buffer<Params> params;

	// Create and precalculate the atmosphere data
	Atmosphere(vuk::Allocator& allocator, Params const& params) {
		if (!compiled) {
			compiled = true;

			auto genTransmittancePci = vuk::PipelineBaseCreateInfo();
			constexpr auto genTransmittanceSource = std::to_array<uint>({
#include "spv/sky/genTransmittance.comp.spv"
			});
			genTransmittancePci.add_static_spirv(genTransmittanceSource.data(), genTransmittanceSource.size(), "sky/genTransmittance.comp");
			Vulkan::serv->context.create_named_pipeline("sky/genTransmittance", genTransmittancePci);

			auto genMultiScatteringPci = vuk::PipelineBaseCreateInfo();
			constexpr auto genMultiScatteringSource = std::to_array<uint>({
#include "spv/sky/genMultiScattering.comp.spv"
			});
			genMultiScatteringPci.add_static_spirv(genMultiScatteringSource.data(), genMultiScatteringSource.size(), "sky/genMultiScattering.comp");
			Vulkan::serv->context.create_named_pipeline("sky/genMultiScattering", genMultiScatteringPci);
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
		auto paramsFut = vuk::create_buffer(allocator, vuk::MemoryUsage::eGPUonly,
			vuk::DomainFlagBits::eGraphicsQueue,
			std::span(&params, 1)
		).second;
		rg->attach_in("params", paramsFut);

		rg->add_pass(vuk::Pass{
			.name = "sky/genTransmittance",
			.resources = {
				"params"_buffer >> vuk::eComputeRead,
				"transmittance/blank"_image >> vuk::eComputeWrite >> "transmittance",
			},
			.execute = [](auto& cmd) {
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
			.execute = [](auto& cmd) {
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

		this->params = vuk::Future(rg, "params");
		transmittance = vuk::Future(rg, "transmittance");
		multiScattering = vuk::Future(rg, "multiScattering");
	}

private:
	static inline bool compiled = false;
};

// Rendering of the sky from atmosphere data. Sky views depend on camera position
export class Sky:
	Vulkan
{
public:
	constexpr static auto ViewFormat = vuk::Format::eB10G11R11UfloatPack32;
	constexpr static auto ViewSize = uvec2{192u, 108u};

	constexpr static auto AerialPerspectiveFormat = vuk::Format::eR16G16B16A16Sfloat;
	constexpr static auto AerialPerspectiveSize = uvec3{32u, 32u, 32u};

	vec3 sunDirection = {-0.435286462f, 0.818654716f, 0.374606609f};
	vec3 sunIlluminance = {8.0f, 8.0f, 8.0f};
	float sunPitch = 22_deg;
	float sunYaw = 118_deg;

	// Create a 360-degree view of the sky at the specified world position
	auto createView(Atmosphere const& atmo, vec3 probePos) -> Texture2D<vec3> {
		if (!compiled) {
			compiled = true;

			auto genViewPci = vuk::PipelineBaseCreateInfo();
			constexpr auto genViewSource = std::to_array<uint>({
#include "spv/sky/genView.comp.spv"
			});
			genViewPci.add_static_spirv(genViewSource.data(), genViewSource.size(), "sky/genView.comp");
			
			Vulkan::serv->context.create_named_pipeline("sky/genView", genViewPci);
		}

		auto rg = std::make_shared<vuk::RenderGraph>("sky");
		rg->attach_image("view/blank", vuk::ImageAttachment{
			.extent = vuk::Dimension3D::absolute(ViewSize.x(), ViewSize.y()),
			.format = ViewFormat,
			.sample_count = vuk::Samples::e1,
			.level_count = 1,
			.layer_count = 1,
		});
		rg->attach_in("params", atmo.params);
		rg->attach_in("transmittance", atmo.transmittance);
		rg->attach_in("multiScattering", atmo.multiScattering);

		rg->add_pass(vuk::Pass{
			.name = "sky/genView",
			.resources = {
				"params"_buffer >> vuk::eComputeRead,
				"transmittance"_image >> vuk::eComputeSampled,
				"multiScattering"_image >> vuk::eComputeSampled,
				"view/blank"_image >> vuk::eComputeWrite >> "view",
			},
			.execute = [this, probePos](vuk::CommandBuffer& cmd) {
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
					.probePos = probePos,
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

private:
	static inline bool compiled = false;
};
