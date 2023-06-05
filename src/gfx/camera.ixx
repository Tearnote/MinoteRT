export module minote.camera;

import <cmath>;
import minote.math;

// A user-controllable camera. Easy to manipulate with intuitive functions,
// and can be converted into transform matrices
export class Camera {
public:
	// Projection
	uvec2 viewport;
	float verticalFov;
	float nearPlane;

	// View
	vec3 position;
	float yaw;
	float pitch;

	// Movement
	float lookSpeed;
	float moveSpeed;

	// Return a unit vector of the direction the camera is pointing in
	[[nodiscard]]
	auto direction() const -> vec3 {
		return vec3{
			std::cos(pitch) * std::cos(yaw),
			std::cos(pitch) * std::sin(yaw),
			std::sin(pitch)
		};
	}

	// Return a matrix that transforms from world space to view space
	[[nodiscard]]
	auto view() const -> mat4 {
		return look(position, direction(), vec3{0.0f, 0.0f, 1.0f});
	}

	// Return a matrix that transforms from view space to clip space
	[[nodiscard]]
	auto projection() const -> mat4 {
		return perspective(verticalFov, float(viewport.y()) / float(viewport.x()), nearPlane);
	}

	// Change camera direction by the provided offsets, taking into account lookSpeed
	void rotate(float horz, float vert) {
		yaw -= horz * lookSpeed;
		if (yaw <    0_deg) yaw += 360_deg;
		if (yaw >= 360_deg) yaw -= 360_deg;

		pitch += vert * lookSpeed;
		pitch = clamp(pitch, -89_deg, 89_deg);
	}

	// Change the camera position directly, taking into account moveSpeed
	void shift(vec3 distance) { position += distance * moveSpeed; }

	// Change the camera position relatively to its direction, taking into account moveSpeed
	void roam(vec3 distance) {
		distance = vec3(inverse(view()) * vec4(distance, 0.0));
		shift(distance);
	}

};
