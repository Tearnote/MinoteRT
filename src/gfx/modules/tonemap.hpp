#pragma once

#include <array>

#include <vuk/Future.hpp>

#include "types.hpp"

namespace minote::gfx::modules {

auto tonemapLinear(vuk::Future input, float exposure) -> vuk::Future;

auto tonemapAces(vuk::Future input, float exposure) -> vuk::Future;

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

auto tonemapUchimura(vuk::Future input, float exposure, UchimuraParams const&) -> vuk::Future;

}
