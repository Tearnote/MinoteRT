#pragma once

#include <vuk/Allocator.hpp>
#include <vuk/resources/DeviceFrameResource.hpp>

#include "camera.hpp"

namespace minote::gfx {

class Renderer {

public:

	constexpr static auto InflightFrames = 3u;

	Renderer();

	void draw(gfx::Camera const&);

private:

	vuk::DeviceSuperFrameResource m_deviceResource;
	vuk::Allocator m_multiFrameAllocator;
	gfx::Camera m_prevCamera;

};

}
