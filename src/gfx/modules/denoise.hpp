#pragma once

#include <array>

#include <vuk/Future.hpp>

#include "types.hpp"

namespace minote::gfx::modules {

struct BilateralParams {
	float sigma;
	float kSigma;
	float threshold;

	static auto make_default() -> BilateralParams {
		return BilateralParams{
			.sigma = 5.0f,
			.kSigma = 2.0f,
			.threshold = 0.12f,
		};
	}
};

auto denoiseBilateral(vuk::Future color, vuk::Future depth, BilateralParams) -> vuk::Future;

}
