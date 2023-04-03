#include "gfx/camera.hpp"

namespace minote::gfx {

using namespace math_literals;

auto Camera::direction() const -> vec3 {

	return vec3{
		cos(pitch) * cos(yaw),
		cos(pitch) * sin(yaw),
		sin(pitch)
	};

}

auto Camera::view() const -> mat4 {

    return look(position, direction(), vec3{0.0f, 0.0f, 1.0f});

}

void Camera::rotate(float horz, float vert) {

	yaw -= horz * lookSpeed;
	if (yaw <    0_deg) yaw += 360_deg;
	if (yaw >= 360_deg) yaw -= 360_deg;

	pitch += vert * lookSpeed;
	pitch = clamp(pitch, -89_deg, 89_deg);

}

void Camera::shift(vec3 distance) {

	position += distance * moveSpeed;

}

void Camera::roam(vec3 distance) {

	distance = vec3(inverse(view()) * vec4(distance, 0.0));
	shift(distance);

}

}
