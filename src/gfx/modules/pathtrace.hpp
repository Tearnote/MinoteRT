#pragma once

#include <vuk/Future.hpp>

#include "math.hpp"
#include "gfx/camera.hpp"

namespace minote::gfx::modules {

struct GBuffer {
	vuk::Future visibility;
	vuk::Future depth;
	vuk::Future normal;
	vuk::Future seed;
	vuk::Future motion;
};

auto primaryRays(uvec2 size, Camera const& camera, Camera const& prevCamera) -> GBuffer;

auto secondaryRays(GBuffer&&, Camera const&) -> vuk::Future;

}
