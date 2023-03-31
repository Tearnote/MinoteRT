#pragma once

#include <array>

#include <vuk/Future.hpp>

#include "types.hpp"

namespace minote::gfx::modules {

auto tonemapLinear(vuk::Future input, float exposure) -> vuk::Future;

auto tonemapReinhard(vuk::Future input, float exposure, float hdrMax) -> vuk::Future;

auto tonemapHable(vuk::Future input, float exposure) -> vuk::Future;

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

auto tonemapAMD(vuk::Future input, float exposure, AMDParams const&) -> vuk::Future;

}
