#pragma once

#include <vuk/resources/DeviceFrameResource.hpp>
#include <vuk/Allocator.hpp>

#include "types.hpp"
#include "stx/time.hpp"
#include "gfx/camera.hpp"
#include "gfx/imgui.hpp"


namespace minote::gfx {

using namespace stx::time_literals;

class Renderer {

public:

	constexpr static auto InflightFrames = 3u;
	static constexpr auto FrameTimeUpdate = 1_s;

	Renderer();

	void draw(gfx::Camera const&);

private:

	vuk::DeviceSuperFrameResource m_deviceResource;
	vuk::Allocator m_multiFrameAllocator;
	gfx::Camera m_prevCamera;
	Imgui m_imgui;

	float m_frameTime;
	stx::nsec m_lastFrameTimeCheck;
	uint m_framesSinceLastCheck;

	vuk::Texture m_blueNoise;

	void updateFrameTime();
	auto denoise(vuk::Future color, vuk::Future depth) -> vuk::Future;
	auto tonemap(vuk::Future) -> vuk::Future;
	void blitAndPresent(vuk::Future, vuk::Allocator& _allocator);

};

}
