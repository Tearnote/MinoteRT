#pragma once

#include <array>

#include <vuk/Future.hpp>

#include "types.hpp"

namespace minote::gfx::modules {

auto tonemapLinear(vuk::Future input, float exposure) -> vuk::Future;

auto tonemapUchimura(vuk::Future input, float exposure) -> vuk::Future;

}
