#pragma once

#include <vuk/Descriptor.hpp> // vuk bug
#include <vuk/Query.hpp>      // vuk bug
#include <vuk/Allocator.hpp>
#include <vuk/Image.hpp>

#include "math.hpp"

namespace minote::gfx {

class Imgui {

public:

	explicit Imgui(vuk::Allocator&);
	~Imgui();

	void begin(uvec2 size);

	auto render(vuk::Allocator&, vuk::Future) -> vuk::Future;

private:

	vuk::Texture m_font;

};

}
