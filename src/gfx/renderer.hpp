#pragma once

#include <vuk/Allocator.hpp>
#include <vuk/resources/DeviceFrameResource.hpp>

namespace minote::gfx {

class Renderer {

public:

	constexpr static auto InflightFrames = 3u;

	Renderer();

	void draw();

private:

	vuk::DeviceSuperFrameResource m_deviceResource;
	vuk::Allocator m_multiFrameAllocator;

};

}
