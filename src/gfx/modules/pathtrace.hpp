#pragma once

#include <vuk/Future.hpp>

#include "math.hpp"
#include "gfx/camera.hpp"
#include "gfx/modules/sky.hpp"

namespace minote::gfx::modules {

struct GBuffer {
	vuk::Future visibility;
	vuk::Future depth;
	vuk::Future normal;
	vuk::Future motion;
};

auto primaryRays(uvec2 size, Camera const& camera, Camera const& prevCamera) -> GBuffer;

auto secondaryRays(GBuffer, Camera const&, Atmosphere const&, Texture2D<vec3> skyView, vuk::Texture& blueNoise) -> vuk::Future;

}
