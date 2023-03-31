#pragma once

#include <array>

#include <vuk/Future.hpp>

#include "types.hpp"

namespace minote::gfx::modules {

enum class TonemapMode: int {
	Linear = 0,
	Uchimura = 1,
};

inline constexpr static auto TonemapModeStrings = std::to_array<const char*>({
	"Linear",
	"Uchimura",
});

auto tonemap(vuk::Future input, TonemapMode) -> vuk::Future;

}
