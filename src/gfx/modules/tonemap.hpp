#pragma once

#include <array>

#include <vuk/Future.hpp>

#include "types.hpp"

namespace minote::gfx::modules {

auto tonemapLinear(vuk::Future input, float exposure) -> vuk::Future;

struct UchimuraParams {
	float maxBrightness;
	float contrast;
	float linearStart;
	float linearLength;
	float blackLevel;
	float pedestal;

	static auto make_default() -> UchimuraParams {
		return UchimuraParams{
			.maxBrightness = 1.0f,
			.contrast = 1.0f,
			.linearStart = 0.16f,
			.linearLength = 0.2f,
			.blackLevel = 1.2f,
			.pedestal = 0.0f,
		};
	}
};

auto tonemapUchimura(vuk::Future input, float exposure, UchimuraParams const&) -> vuk::Future;

}
