#pragma once

#include <vuk/resources/DeviceFrameResource.hpp>
#include <vuk/Allocator.hpp>

#include "types.hpp"
#include "stx/time.hpp"
#include "util/service.hpp"
#include "gfx/camera.hpp"
#include "gfx/imgui.hpp"


namespace minote::gfx {

using namespace stx::time_literals;

class Renderer {

public:

	constexpr static auto InflightFrames = 3u;
	static constexpr auto FrameTimeUpdate = 1_s;

	void draw(gfx::Camera const&);

	[[nodiscard]]
	auto frameTime() const -> float { return m_frameTime; }

	// Not movable, not copyable
	Renderer(Renderer const&) = delete;
	auto operator=(Renderer const&) -> Renderer& = delete;

private:

	vuk::DeviceSuperFrameResource m_deviceResource;
	vuk::Allocator m_multiFrameAllocator;
	gfx::Camera m_prevCamera;
	Imgui m_imgui;

	float m_frameTime;
	stx::nsec m_lastFrameTimeCheck;
	uint m_framesSinceLastCheck;

	vuk::Texture m_blueNoise;

	// Can only be used as service
	friend struct util::Service<Renderer>;
	Renderer();

	void updateFrameTime();
	auto denoise(vuk::Future color, vuk::Future depth, vuk::Future normal, Camera const&) -> vuk::Future;
	auto tonemap(vuk::Future) -> vuk::Future;
	void blitAndPresent(vuk::Future, vuk::Allocator& _allocator);

};

inline util::Service<Renderer> s_renderer;

}
