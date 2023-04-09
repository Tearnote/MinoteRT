#pragma once

#include <string_view>

#include <vuk/Future.hpp>

#include "math.hpp"
#include "gfx/renderer.hpp"
#include "gfx/resource.hpp"
#include "gfx/camera.hpp"

namespace minote::gfx::modules {

using namespace math_literals;

// Precalculated representation of a planet's atmosphere. Once created, it can
// be used repeatedly to sample the sky at any elevation and sun position
class Atmosphere {

public:

	constexpr static auto TransmittanceFormat = vuk::Format::eR16G16B16A16Sfloat;
	constexpr static auto TransmittanceSize = uvec2{256u, 64u};

	constexpr static auto MultiScatteringFormat = vuk::Format::eR16G16B16A16Sfloat;
	constexpr static auto MultiScatteringSize = uvec2{32u, 32u};

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
		static auto earth() -> Params;
	};

	Texture2D<vec4> transmittance;
	Texture2D<vec4> multiScattering;
	Buffer<Params> params;

	// Create and precalculate the atmosphere data
	Atmosphere(vuk::Allocator&, Params const&);

};

// Rendering of the sky from atmosphere data. Sky views depend on camera position
class Sky {

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
	auto createView(Atmosphere const&, vec3 probePos) -> Texture2D<vec3>;

};

}
