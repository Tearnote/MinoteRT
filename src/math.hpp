#pragma once

#include <type_traits>
#include <concepts>
#include <numbers>
#include <array>
#include <cmath>

#include "types.hpp"
#include "stx/concepts.hpp"

// Avoid collisions with common min/max macros
#ifdef min
#undef min
#endif //min
#ifdef max
#undef max
#endif //max

namespace minote {

//=== Constants

template<std::floating_point T>
constexpr auto Pi_v = std::numbers::pi_v<T>;
constexpr auto Pi = Pi_v<float>;

template<std::floating_point T>
constexpr auto Tau_v = Pi_v<T> * T(2.0);
constexpr auto Tau = Tau_v<float>;

//=== Scalar operations

using std::min;
using std::max;
using std::abs;
using std::round;
using std::floor;
using std::ceil;

using std::pow;
using std::sqrt;
using std::log2;

using std::sin;
using std::cos;
using std::tan;

// Degrees to radians conversion
template<stx::arithmetic T, std::floating_point Prec = float>
constexpr auto radians(T deg) -> Prec { return Prec(deg) * Tau_v<Prec> / Prec(360); }

// True modulo operation (as opposed to remainder, which is operator% in C++.)
// The result is always positive and does not flip direction at zero.
// Example:
//  5 mod 4 = 1
//  4 mod 4 = 0
//  3 mod 4 = 3
//  2 mod 4 = 2
//  1 mod 4 = 1
//  0 mod 4 = 0
// -1 mod 4 = 3
// -2 mod 4 = 2
// -3 mod 4 = 1
// -4 mod 4 = 0
// -5 mod 4 = 3
template<std::integral T>
constexpr auto tmod(T num, T div) { return num % div + (num % div < 0) * div; }

// GLSL-style scalar clamp
template<stx::arithmetic T>
constexpr auto clamp(T val, T vmin, T vmax) -> T { return max(vmin, min(val, vmax)); }

//=== Compound types

// Generic math vector, of any dimension between 2 to 4 and any underlying type
template<usize Dim, stx::arithmetic T>
struct vec {

	static_assert(Dim >= 2 && Dim <= 4, "Vectors need to have 2, 3 or 4 components");

	//=== Creation

	// Uninitialized init
	constexpr vec() = default;

	// Fill the vector with copies of the value
	explicit constexpr vec(T fillVal) { fill(fillVal); }

	// Create the vector with provided component values
	constexpr vec(std::initializer_list<T>);

	//=== Conversions

	// Type cast
	template<stx::arithmetic U>
	requires (!std::same_as<T, U>)
	explicit constexpr vec(vec<Dim, U> const&);

	// Dimension downcast
	template<usize N>
	requires (N > Dim)
	explicit constexpr vec(vec<N, T> const&);

	// Dimension upcast
	template<usize N>
	requires (N < Dim)
	constexpr vec(vec<N, T> const&, T fill);

	//=== Member access

	[[nodiscard]]
	constexpr auto at(usize n) -> T& { return m_arr[n]; }
	[[nodiscard]]
	constexpr auto at(usize n) const -> T { return m_arr[n]; }

	[[nodiscard]]
	constexpr auto operator[](usize n) -> T& { return at(n); }
	[[nodiscard]]
	constexpr auto operator[](usize n) const -> T { return at(n); }

	[[nodiscard]]
	constexpr auto x() -> T& { static_assert(Dim >= 1); return m_arr[0]; }
	[[nodiscard]]
	constexpr auto x() const -> T { static_assert(Dim >= 1); return m_arr[0]; }
	[[nodiscard]]
	constexpr auto y() -> T& { static_assert(Dim >= 2); return m_arr[1]; }
	[[nodiscard]]
	constexpr auto y() const -> T { static_assert(Dim >= 2); return m_arr[1]; }
	[[nodiscard]]
	constexpr auto z() -> T& { static_assert(Dim >= 3); return m_arr[2]; }
	[[nodiscard]]
	constexpr auto z() const -> T { static_assert(Dim >= 3); return m_arr[2]; }
	[[nodiscard]]
	constexpr auto w() -> T& { static_assert(Dim >= 4); return m_arr[3]; }
	[[nodiscard]]
	constexpr auto w() const -> T { static_assert(Dim >= 4); return m_arr[3]; }

	[[nodiscard]]
	constexpr auto r() -> T& { return x(); }
	[[nodiscard]]
	constexpr auto r() const -> T { return x(); }
	[[nodiscard]]
	constexpr auto g() -> T& { return y(); }
	[[nodiscard]]
	constexpr auto g() const -> T { return y(); }
	[[nodiscard]]
	constexpr auto b() -> T& { return z(); }
	[[nodiscard]]
	constexpr auto b() const -> T { return z(); }
	[[nodiscard]]
	constexpr auto a() -> T& { return w(); }
	[[nodiscard]]
	constexpr auto a() const -> T { return w(); }

	[[nodiscard]]
	constexpr auto u() -> T& { return x(); }
	[[nodiscard]]
	constexpr auto u() const -> T { return x(); }
	[[nodiscard]]
	constexpr auto v() -> T& { return y(); }
	[[nodiscard]]
	constexpr auto v() const -> T { return y(); }
	[[nodiscard]]
	constexpr auto s() -> T& { return z(); }
	[[nodiscard]]
	constexpr auto s() const -> T { return z(); }
	[[nodiscard]]
	constexpr auto t() -> T& { return w(); }
	[[nodiscard]]
	constexpr auto t() const -> T { return w(); }

	constexpr void fill(T val) { m_arr.fill(val); }

	//=== Vector operations

	// Component-wise arithmetic

	constexpr auto operator+=(vec<Dim, T> const&) -> vec<Dim, T>&;
	constexpr auto operator-=(vec<Dim, T> const&) -> vec<Dim, T>&;
	constexpr auto operator*=(vec<Dim, T> const&) -> vec<Dim, T>&; // Component-wise
	constexpr auto operator/=(vec<Dim, T> const&) -> vec<Dim, T>&;
	constexpr auto operator%=(vec<Dim, T> const&) -> vec<Dim, T>&;

	// Scalar arithmetic

	constexpr auto operator*=(T) -> vec<Dim, T>&;
	constexpr auto operator/=(T) -> vec<Dim, T>&;
	constexpr auto operator%=(T) -> vec<Dim, T>&;
	constexpr auto operator<<=(T) -> vec<Dim, T>&;
	constexpr auto operator>>=(T) -> vec<Dim, T>&;

private:

	std::array<T, Dim> m_arr;

};

// Binary vector operations

template<usize Dim, stx::arithmetic T>
constexpr auto operator+(vec<Dim, T> const&, vec<Dim, T> const&) -> vec<Dim, T>;

template<usize Dim, stx::arithmetic T>
constexpr auto operator-(vec<Dim, T> const&, vec<Dim, T> const&) -> vec<Dim, T>;

template<usize Dim, stx::arithmetic T>
constexpr auto operator*(vec<Dim, T> const&, vec<Dim, T> const&) -> vec<Dim, T>; // Component-wise

template<usize Dim, stx::arithmetic T>
constexpr auto operator/(vec<Dim, T> const&, vec<Dim, T> const&) -> vec<Dim, T>;

template<usize Dim, std::integral T>
constexpr auto operator%(vec<Dim, T> const&, vec<Dim, T> const&) -> vec<Dim, T>;

template<usize Dim, stx::arithmetic T>
constexpr auto operator==(vec<Dim, T> const&, vec<Dim, T> const&) -> bool;

template<usize Dim, stx::arithmetic T>
constexpr auto min(vec<Dim, T> const&, vec<Dim, T> const&) -> vec<Dim, T>;

template<usize Dim, stx::arithmetic T>
constexpr auto max(vec<Dim, T> const&, vec<Dim, T> const&) -> vec<Dim, T>;

template<usize Dim, stx::arithmetic T>
constexpr auto dot(vec<Dim, T> const&, vec<Dim, T> const&) -> T;

template<stx::arithmetic T>
constexpr auto cross(vec<3, T> const&, vec<3, T> const&) -> vec<3, T>;

// Binary scalar operations

template<usize Dim, stx::arithmetic T>
constexpr auto operator*(vec<Dim, T> const&, T) -> vec<Dim, T>;
template<usize Dim, stx::arithmetic T>
constexpr auto operator*(T left, vec<Dim, T> const& right) -> vec<Dim, T> { return right * left; }

template<usize Dim, stx::arithmetic T>
constexpr auto operator/(vec<Dim, T> const&, T) -> vec<Dim, T>;

template<usize Dim, std::integral T>
constexpr auto operator%(vec<Dim, T> const&, T) -> vec<Dim, T>;

template<usize Dim, std::integral T>
constexpr auto operator<<(vec<Dim, T> const&, T) -> vec<Dim, T>;

template<usize Dim, std::integral T>
constexpr auto operator>>(vec<Dim, T> const&, T) -> vec<Dim, T>;

// Unary vector operations

// Component-wise absolute value
template<usize Dim, std::floating_point T>
constexpr auto abs(vec<Dim, T> const&) -> vec<Dim, T>;

// Vector length as Euclidean distance
template<usize Dim, std::floating_point T>
constexpr auto length(vec<Dim, T> const& v) -> T { return sqrt(length2(v)); }

// Square of vector length (faster to compute than length)
template<usize Dim, stx::arithmetic T>
constexpr auto length2(vec<Dim, T> const& v) -> T { return dot(v, v); }

// true if vector has the length of 1 (within reasonable epsilon)
template<usize Dim, std::floating_point T>
constexpr auto isUnit(vec<Dim, T> const& v) -> bool { return (abs(length2(v) - 1) < (1.0 / 16.0)); }

// Constructs a vector in the same direction but length 1
template<usize Dim, std::floating_point T>
constexpr auto normalize(vec<Dim, T> const&) -> vec<Dim, T>;

//=== GLSL-like vector aliases

using vec2 = vec<2, float>;
using vec3 = vec<3, float>;
using vec4 = vec<4, float>;
using ivec2 = vec<2, int>;
using ivec3 = vec<3, int>;
using ivec4 = vec<4, int>;
using uvec2 = vec<2, uint>;
using uvec3 = vec<3, uint>;
using uvec4 = vec<4, uint>;
using u8vec2 = vec<2, uint8>;
using u8vec3 = vec<3, uint8>;
using u8vec4 = vec<4, uint8>;
using u16vec2 = vec<2, uint16>;
using u16vec3 = vec<3, uint16>;
using u16vec4 = vec<4, uint16>;

static_assert(std::is_trivially_constructible_v<vec2>);
static_assert(std::is_trivially_constructible_v<vec3>);
static_assert(std::is_trivially_constructible_v<vec4>);
static_assert(std::is_trivially_constructible_v<ivec2>);
static_assert(std::is_trivially_constructible_v<ivec3>);
static_assert(std::is_trivially_constructible_v<ivec4>);
static_assert(std::is_trivially_constructible_v<uvec2>);
static_assert(std::is_trivially_constructible_v<uvec3>);
static_assert(std::is_trivially_constructible_v<uvec4>);
static_assert(std::is_trivially_constructible_v<u8vec2>);
static_assert(std::is_trivially_constructible_v<u8vec3>);
static_assert(std::is_trivially_constructible_v<u8vec4>);
static_assert(std::is_trivially_constructible_v<u16vec2>);
static_assert(std::is_trivially_constructible_v<u16vec3>);
static_assert(std::is_trivially_constructible_v<u16vec4>);

// Quaternion, equivalent to a vec4 but with unique operations available.
// Main purpose is representing rotations. Data layout is {w, x, y, z}.
template<std::floating_point Prec = float>
struct qua {

	//=== Creation

	// Uninitialized init
	constexpr qua() = default;

	// Create the quaternion with provided {w, x, y, z} values
	constexpr qua(std::initializer_list<Prec>);

	// Convert a position vector into a quaternion
	template<usize N>
	requires (N == 3 || N == 4)
	constexpr explicit qua(vec<N, Prec> const&);

	// Create a unit quaternion that represents no rotation
	static constexpr auto identity() -> qua<Prec> { return qua<Prec>{1, 0, 0, 0}; }

	// Create a unit quaternion that represents a rotation around an arbitrary axis
	static constexpr auto angleAxis(Prec angle, vec<3, Prec> axis) -> qua<Prec>;

	//=== Conversion

	// Type cast
	template<stx::arithmetic U>
	requires (!std::same_as<Prec, U>)
	explicit constexpr qua(qua<U> const&);

	//=== Member access

	constexpr auto at(usize n) -> Prec& { return m_arr[n]; }
	constexpr auto at(usize n) const -> Prec { return m_arr[n]; }

	constexpr auto operator[](usize n) -> Prec& { return at(n); }
	constexpr auto operator[](usize n) const -> Prec { return at(n); }

	constexpr auto w() -> Prec& { return m_arr[0]; }
	constexpr auto w() const -> Prec { return m_arr[0]; }
	constexpr auto x() -> Prec& { return m_arr[1]; }
	constexpr auto x() const -> Prec { return m_arr[1]; }
	constexpr auto y() -> Prec& { return m_arr[2]; }
	constexpr auto y() const -> Prec { return m_arr[2]; }
	constexpr auto z() -> Prec& { return m_arr[3]; }
	constexpr auto z() const -> Prec { return m_arr[3]; }

private:

	std::array<Prec, 4> m_arr;

};

// Binary quaternion operations

template<std::floating_point Prec>
constexpr auto operator*(qua<Prec> const&, qua<Prec> const&) -> qua<Prec>;

//=== Quaternion alias

using quat = qua<float>;


// Generic matrix type, of order 3 or 4, and any floating-point precision
template<usize Dim, std::floating_point Prec>
struct mat {

	using col_t = vec<Dim, Prec>;

	static_assert(Dim >= 3 && Dim <= 4, "Matrices need to have order 3x3 or 4x4");

	//=== Creation

	// Uninitialized init
	constexpr mat() = default;

	// Compose a matrix out of all component values, in column-major order
	constexpr mat(std::initializer_list<Prec>);

	// Compose a matrix out of column vectors
	constexpr mat(std::initializer_list<col_t> list) { std::copy(list.begin(), list.end(), m_arr.begin()); }

	// Create a matrix that is a no-op on multiplication
	static constexpr auto identity() -> mat<Dim, Prec>;

	// Classic translation, rotation and scale matrices for vector manipulation

	static constexpr auto translate(vec<3, Prec> shift) -> mat<Dim, Prec>; // Cannot be mat3
	static constexpr auto rotate(vec<3, Prec> axis, Prec angle) -> mat<Dim, Prec>;
	static constexpr auto rotate(qua<Prec> quat) -> mat<Dim, Prec>;
	static constexpr auto scale(vec<3, Prec> scale) -> mat<Dim, Prec>;
	static constexpr auto scale(Prec scale) -> mat<Dim, Prec>;

	//=== Conversion

	// Type cast
	template<stx::arithmetic U>
	requires (!std::same_as<Prec, U>)
	explicit constexpr mat(mat<Dim, U> const&);

	// Dimension cast
	template<usize N>
	requires (N != Dim)
	explicit constexpr mat(mat<N, Prec> const&);

	//=== Member access

	constexpr auto at(usize x, usize y) -> Prec& { return m_arr[x][y]; }
	constexpr auto at(usize x, usize y) const -> Prec { return m_arr[x][y]; }

	constexpr auto operator[](usize x) -> col_t& { return m_arr[x]; }
	constexpr auto operator[](usize x) const -> col_t const& { return m_arr[x]; }

	constexpr auto fill(Prec val) { for (auto& col: m_arr) col.fill(val); }

	//=== Operations

	// Scalar arithmetic

	constexpr auto operator*=(Prec) -> mat<Dim, Prec>&; // Component-wise
	constexpr auto operator/=(Prec) -> mat<Dim, Prec>&; // Component-wise

private:

	std::array<col_t, Dim> m_arr;

};

// Binary matrix operations

template<usize Dim, std::floating_point Prec>
constexpr auto operator*(mat<Dim, Prec> const&, mat<Dim, Prec> const&) -> mat<Dim, Prec>;

template<usize Dim, std::floating_point Prec>
constexpr auto operator*(mat<Dim, Prec> const&, vec<Dim, Prec> const&) -> vec<Dim, Prec>;

template<usize Dim, std::floating_point Prec>
constexpr auto operator*(mat<Dim, Prec> const&, Prec) -> mat<Dim, Prec>; // Component-wise
template<usize Dim, std::floating_point Prec>
constexpr auto operator*(Prec left, mat<Dim, Prec> const& right) -> mat<Dim, Prec> { return right * left; } // Component-wise

template<usize Dim, std::floating_point Prec>
constexpr auto operator/(mat<Dim, Prec> const&, Prec) -> mat<Dim, Prec>; // Component-wise

// Unary matrix operations

// Creates a matrix with rows transposed with columns
template<usize Dim, std::floating_point Prec>
constexpr auto transpose(mat<Dim, Prec> const&) -> mat<Dim, Prec>;

// Creates a matrix that results in identity when multiplied with the original (slow!)
template<usize Dim, std::floating_point Prec>
constexpr auto inverse(mat<Dim, Prec> const&) -> mat<Dim, Prec>;

// Specialized matrix generators

// Variant of lookAt matrix. Dir is a unit vector of the camera direction.
// Dir and Up are both required to be unit vectors
template<std::floating_point Prec = float>
constexpr auto look(vec<3, Prec> pos, vec<3, Prec> dir, vec<3, Prec> up) -> mat<4, Prec>;

// Creates a perspective matrix. The matrix uses inverted infinite depth:
// 1.0 at zNear, 0.0 at infinity.
template<std::floating_point Prec = float>
constexpr auto perspective(Prec vFov, Prec aspectRatio, Prec zNear) -> mat<4, Prec>;

//=== GLSL-like matrix aliases

using mat3 = mat<3, float>;
using mat4 = mat<4, float>;

static_assert(std::is_trivially_constructible_v<mat3>);
static_assert(std::is_trivially_constructible_v<mat4>);

//=== Conversion literals

namespace math_literals {

consteval auto operator ""_cm(unsigned long long int val) -> float { return double(val) * 0.000'001; }

consteval auto operator ""_cm(long double val) -> float { return double(val) * 0.000'001; }

consteval auto operator ""_m(unsigned long long int val) -> float { return double(val) * 0.001; }

consteval auto operator ""_m(long double val) -> float { return double(val) * 0.001; }

consteval auto operator ""_km(unsigned long long int val) -> float { return val; }

consteval auto operator ""_km(long double val) -> float { return val; }

consteval auto operator ""_deg(unsigned long long int val) -> float { return radians(double(val)); }

consteval auto operator ""_deg(long double val) -> float { return radians(val); }

}

}

// TEMPLATE IMPLEMENTATION
// Some algorithms are adapted from GLM code:
// https://github.com/g-truc/glm

#include <algorithm>

#include "stx/ranges.hpp"
#include "stx/verify.hpp"

namespace minote {

using namespace type_literals;

template<usize Dim, stx::arithmetic T>
constexpr vec<Dim, T>::vec(std::initializer_list<T> _list) {

	ASSUME(_list.size() == m_arr.size());
	std::copy(_list.begin(), _list.end(), m_arr.begin());

}

template<usize Dim, stx::arithmetic T>
template<stx::arithmetic U>
requires (!std::same_as<T, U>)
constexpr vec<Dim, T>::vec(vec<Dim, U> const& _other) {

	for (auto i: stx::iota(0_zu, Dim))
		m_arr[i] = T(_other[i]);

}

template<usize Dim, stx::arithmetic T>
template<usize N>
requires (N > Dim)
constexpr vec<Dim, T>::vec(vec<N, T> const& _other) {

	for (auto i: stx::iota(0_zu, Dim))
		m_arr[i] = _other[i];

}

template<usize Dim, stx::arithmetic T>
template<usize N>
requires (N < Dim)
constexpr vec<Dim, T>::vec(vec<N, T> const& _other, T _fill) {

	m_arr.fill(_fill);
	for (auto i: stx::iota(0_zu, N))
		m_arr[i] = _other[i];

}

template<usize Dim, stx::arithmetic T>
constexpr auto vec<Dim, T>::operator+=(vec<Dim, T> const& _other) -> vec<Dim, T>& {

	for (auto i: stx::iota(0_zu, Dim))
		m_arr[i] += _other[i];
	return *this;

}

template<usize Dim, stx::arithmetic T>
constexpr auto vec<Dim, T>::operator-=(vec<Dim, T> const& _other) -> vec<Dim, T>& {

	for (auto i: stx::iota(0_zu, Dim))
		m_arr[i] -= _other[i];
	return *this;

}

template<usize Dim, stx::arithmetic T>
constexpr auto vec<Dim, T>::operator*=(vec<Dim, T> const& _other) -> vec<Dim, T>& {

	for (auto i: stx::iota(0_zu, Dim))
		m_arr[i] *= _other[i];
	return *this;

}

template<usize Dim, stx::arithmetic T>
constexpr auto vec<Dim, T>::operator/=(vec<Dim, T> const& _other) -> vec<Dim, T>& {

	for (auto i: stx::iota(0_zu, Dim))
		m_arr[i] /= _other[i];
	return *this;

}

template<usize Dim, stx::arithmetic T>
constexpr auto vec<Dim, T>::operator%=(vec<Dim, T> const& _other) -> vec<Dim, T>& {

	static_assert(std::is_integral_v<T>);

	for (auto i: stx::iota(0_zu, Dim))
		m_arr[i] /= _other[i];
	return *this;

}

template<usize Dim, stx::arithmetic T>
constexpr auto vec<Dim, T>::operator*=(T _other) -> vec<Dim, T>& {

	for (auto i: stx::iota(0_zu, Dim))
		m_arr[i] *= _other;
	return *this;

}

template<usize Dim, stx::arithmetic T>
constexpr auto vec<Dim, T>::operator/=(T _other) -> vec<Dim, T>& {

	for (auto i: stx::iota(0_zu, Dim))
		m_arr[i] /= _other;
	return *this;

}

template<usize Dim, stx::arithmetic T>
constexpr auto vec<Dim, T>::operator%=(T _other) -> vec<Dim, T>& {

	static_assert(std::is_integral_v<T>);

	for (auto i: stx::iota(0_zu, Dim))
		m_arr[i] %= _other;
	return *this;

}

template<usize Dim, stx::arithmetic T>
constexpr auto vec<Dim, T>::operator<<=(T _other) -> vec<Dim, T>& {

	static_assert(std::is_integral_v<T>);

	for (auto i: stx::iota(0_zu, Dim))
		m_arr[i] <<= _other;
	return *this;

}

template<usize Dim, stx::arithmetic T>
constexpr auto vec<Dim, T>::operator>>=(T _other) -> vec<Dim, T>& {

	static_assert(std::is_integral_v<T>);

	for (auto i: stx::iota(0_zu, Dim))
		m_arr[i] >>= _other;
	return *this;

}

template<usize Dim, stx::arithmetic T>
constexpr auto operator+(vec<Dim, T> const& _left, vec<Dim, T> const& _right) -> vec<Dim, T> {

	auto result = _left;
	result += _right;
	return result;

}

template<usize Dim, stx::arithmetic T>
constexpr auto operator-(vec<Dim, T> const& _left, vec<Dim, T> const& _right) -> vec<Dim, T> {

	auto result = _left;
	result -= _right;
	return result;

}

template<usize Dim, stx::arithmetic T>
constexpr auto operator*(vec<Dim, T> const& _left, vec<Dim, T> const& _right) -> vec<Dim, T> {

	auto result = _left;
	result *= _right;
	return result;

}

template<usize Dim, stx::arithmetic T>
constexpr auto operator/(vec<Dim, T> const& _left, vec<Dim, T> const& _right) -> vec<Dim, T> {

	auto result = _left;
	result /= _right;
	return result;

}

template<usize Dim, std::integral T>
constexpr auto operator%(vec<Dim, T> const& _left, vec<Dim, T> const& _right) -> vec<Dim, T> {

	auto result = _left;
	result %= _right;
	return result;

}

template<usize Dim, stx::arithmetic T>
constexpr auto operator==(vec<Dim, T> const& _left, vec<Dim, T> const& _right) -> bool {

	for (auto i: stx::iota(0_zu, Dim))
		if (_left[i] != _right[i])
			return false;
	return true;

}

template<usize Dim, stx::arithmetic T>
constexpr auto min(vec<Dim, T> const& _left, vec<Dim, T> const& _right) -> vec<Dim, T> {

	auto result = vec<Dim, T>();
	for (auto i: stx::iota(0_zu, Dim))
		result[i] = min(_left[i], _right[i]);
	return result;

}

template<usize Dim, stx::arithmetic T>
constexpr auto max(vec<Dim, T> const& _left, vec<Dim, T> const& _right) -> vec<Dim, T> {

	auto result = vec<Dim, T>();
	for (auto i: stx::iota(0_zu, Dim))
		result[i] = max(_left[i], _right[i]);
	return result;

}

template<usize Dim, stx::arithmetic T>
constexpr auto dot(vec<Dim, T> const& _left, vec<Dim, T> const& _right) -> T {

	auto result = T(0);
	for (auto i: stx::iota(0_zu, Dim))
		result += _left[i] * _right[i];
	return result;

}

template<stx::arithmetic T>
constexpr auto cross(vec<3, T> const& _left, vec<3, T> const& _right) -> vec<3, T> {

	return vec<3, T>{
		_left[1]*_right[2] - _right[1]*_left[2],
		_left[2]*_right[0] - _right[2]*_left[0],
		_left[0]*_right[1] - _right[0]*_left[1],
	};

}

template<usize Dim, stx::arithmetic T>
constexpr auto operator*(vec<Dim, T> const& _left, T _right) -> vec<Dim, T> {

	auto result = _left;
	result *= _right;
	return result;

}

template<usize Dim, stx::arithmetic T>
constexpr auto operator/(vec<Dim, T> const& _left, T _right) -> vec<Dim, T> {

	auto result = _left;
	result /= _right;
	return result;

}

template<usize Dim, std::integral T>
constexpr auto operator%(vec<Dim, T> const& _left, T _right) -> vec<Dim, T> {

	auto result = _left;
	result %= _right;
	return result;

}

template<usize Dim, std::integral T>
constexpr auto operator<<(vec<Dim, T> const& _left, T _right) -> vec<Dim, T> {

	auto result = _left;
	result <<= _right;
	return result;

}

template<usize Dim, std::integral T>
constexpr auto operator>>(vec<Dim, T> const& _left, T _right) -> vec<Dim, T> {

	auto result = _left;
	result >>= _right;
	return result;

}

template<usize Dim, std::floating_point T>
constexpr auto abs(vec<Dim, T> const& _vec) -> vec<Dim, T> {

	auto result = vec<Dim, T>();
	for (auto i: stx::iota(0_zu, Dim))
		result[i] = abs(_vec[i]);
	return result;

}

template<usize Dim, std::floating_point T>
constexpr auto normalize(vec<Dim, T> const& _vec) -> vec<Dim, T> {

	if constexpr (Dim == 4) {
		auto norm = normalize(vec<3, T>(_vec));
		return vec<Dim, T>(norm, _vec[3]);
	}
	return _vec / length(_vec);

}

template<std::floating_point Prec>
constexpr qua<Prec>::qua(std::initializer_list<Prec> _list) {

	ASSUME(_list.size() == m_arr.size());
	std::copy(_list.begin(), _list.end(), m_arr.begin());

}

template<std::floating_point Prec>
template<usize N>
requires (N == 3 || N == 4)
constexpr qua<Prec>::qua(vec<N, Prec> const& _other) {

	m_arr[0] = 0;
	m_arr[1] = _other.x();
	m_arr[2] = _other.y();
	m_arr[3] = _other.z();

}

template<std::floating_point Prec>
constexpr auto qua<Prec>::angleAxis(Prec _angle, vec<3, Prec> _axis) -> qua<Prec> {

	ASSUME(isUnit(_axis));

	auto halfAngle = _angle / Prec(2);
	auto sinHalfAngle = sin(halfAngle);
	return qua<Prec>{
		cos(halfAngle),
		sinHalfAngle * _axis[0],
		sinHalfAngle * _axis[1],
		sinHalfAngle * _axis[2],
	};

}

template<std::floating_point Prec>
template<stx::arithmetic U>
requires (!std::same_as<Prec, U>)
constexpr qua<Prec>::qua(qua<U> const& _other) {

	for (auto i: stx::iota(0_zu, m_arr.size()))
		m_arr[i] = _other[i];

}

template<std::floating_point Prec>
constexpr auto operator*(qua<Prec> const& _l, qua<Prec> const& _r) -> qua<Prec> {

	return qua<Prec>{
		-_l.x() * _r.x() - _l.y() * _r.y() - _l.z() * _r.z() + _l.w() * _r.w(),
		 _l.x() * _r.w() + _l.y() * _r.z() - _l.z() * _r.y() + _l.w() * _r.x(),
		-_l.x() * _r.z() + _l.y() * _r.w() + _l.z() * _r.x() + _l.w() * _r.y(),
		 _l.x() * _r.y() - _l.y() * _r.x() + _l.z() * _r.w() + _l.w() * _r.z(),
	};

}

template<usize Dim, std::floating_point Prec>
constexpr mat<Dim, Prec>::mat(std::initializer_list<Prec> _list) {

	ASSUME(_list.size() == Dim * Dim);

	auto it = _list.begin();
	for (auto x: stx::iota(0_zu, Dim))
		for (auto y: stx::iota(0_zu, Dim)) {
			m_arr[x][y] = *it;
			it += 1;
		}

}

template<usize Dim, std::floating_point Prec>
constexpr auto mat<Dim, Prec>::identity() -> mat<Dim, Prec> {

	auto result = mat<Dim, Prec>();
	result.fill(0);
	for (auto i: stx::iota(0_zu, Dim))
		result[i][i] = 1;
	return result;

}

template<usize Dim, std::floating_point Prec>
constexpr auto mat<Dim, Prec>::translate(vec<3, Prec> _shift) -> mat<Dim, Prec> {

	static_assert(Dim == 4, "Translation matrix requires order of 4");

	auto result = mat<Dim, Prec>::identity();
	result[3][0] = _shift[0];
	result[3][1] = _shift[1];
	result[3][2] = _shift[2];
	return result;

}

template<usize Dim, std::floating_point Prec>
constexpr auto mat<Dim, Prec>::rotate(vec<3, Prec> _axis, Prec _angle) -> mat<Dim, Prec> {

	ASSUME(isUnit(_axis));

	auto sinT = sin(_angle);
	auto cosT = cos(_angle);
	auto temp = _axis * (Prec(1) - cosT);

	auto result = mat<Dim, Prec>::identity();

	result[0][0] = cosT + temp[0] * _axis[0];
	result[0][1] = temp[0] * _axis[1] + sinT * _axis[2];
	result[0][2] = temp[0] * _axis[2] - sinT * _axis[1];

	result[1][0] = temp[1] * _axis[0] - sinT * _axis[2];
	result[1][1] = cosT + temp[1] * _axis[1];
	result[1][2] = temp[1] * _axis[2] + sinT * _axis[0];

	result[2][0] = temp[2] * _axis[0] + sinT * _axis[1];
	result[2][1] = temp[2] * _axis[1] - sinT * _axis[0];
	result[2][2] = cosT + temp[2] * _axis[2];

	return result;

}

template<usize Dim, std::floating_point Prec>
constexpr auto mat<Dim, Prec>::rotate(qua<Prec> _quat) -> mat<Dim, Prec> {

	auto result = mat<Dim, Prec>::identity();

	result[0][0] = 1.0f - 2.0f * (_quat.y() * _quat.y() + _quat.z() * _quat.z());
	result[0][1] =        2.0f * (_quat.x() * _quat.y() + _quat.z() * _quat.w());
	result[0][2] =        2.0f * (_quat.x() * _quat.z() - _quat.y() * _quat.w());

	result[1][0] =        2.0f * (_quat.x() * _quat.y() - _quat.z() * _quat.w());
	result[1][1] = 1.0f - 2.0f * (_quat.x() * _quat.x() + _quat.z() * _quat.z());
	result[1][2] =        2.0f * (_quat.y() * _quat.z() + _quat.x() * _quat.w());

	result[2][0] =        2.0f * (_quat.x() * _quat.z() + _quat.y() * _quat.w());
	result[2][1] =        2.0f * (_quat.y() * _quat.z() - _quat.x() * _quat.w());
	result[2][2] = 1.0f - 2.0f * (_quat.x() * _quat.x() + _quat.y() * _quat.y());

	return result;

}

template<usize Dim, std::floating_point Prec>
constexpr auto mat<Dim, Prec>::scale(vec<3, Prec> _scale) -> mat<Dim, Prec> {

	auto result = mat<Dim, Prec>::identity();
	for (auto i: stx::iota(0_zu, 3_zu))
		result[i][i] = _scale[i];
	return result;

}

template<usize Dim, std::floating_point Prec>
constexpr auto mat<Dim, Prec>::scale(Prec _scale) -> mat<Dim, Prec> {

auto result = mat<Dim, Prec>::identity();
for (auto i: stx::iota(0_zu, 3_zu))
result[i][i] = _scale;
return result;

}

template<usize Dim, std::floating_point Prec>
template<stx::arithmetic U>
requires (!std::same_as<Prec, U>)
constexpr mat<Dim, Prec>::mat(mat<Dim, U> const& _other) {

	for (auto x: stx::iota(0_zu, Dim))
		for (auto y: stx::iota(0_zu, Dim))
			m_arr[x][y] = _other[x][y];

}

template<usize Dim, std::floating_point Prec>
template<usize N>
requires (N != Dim)
constexpr mat<Dim, Prec>::mat(mat<N, Prec> const& _other) {

	if constexpr (Dim > N)
		*this = identity();

	constexpr auto Smaller = min(Dim, N);
	for (auto x: stx::iota(0_zu, Smaller))
		for (auto y: stx::iota(0_zu, Smaller))
			m_arr[x][y] = _other[x][y];

}

template<usize Dim, std::floating_point Prec>
constexpr auto mat<Dim, Prec>::operator*=(Prec _other) -> mat<Dim, Prec>& {

	for (auto x: stx::iota(0_zu, Dim))
		for (auto y: stx::iota(0_zu, Dim))
			m_arr[x][y] *= _other;
	return *this;

}

template<usize Dim, std::floating_point Prec>
constexpr auto mat<Dim, Prec>::operator/=(Prec _other) -> mat<Dim, Prec>& {

	for (auto x: stx::iota(0_zu, Dim))
		for (auto y: stx::iota(0_zu, Dim))
			m_arr[x][y] /= _other;
	return *this;

}

template<usize Dim, std::floating_point Prec>
constexpr auto operator*(mat<Dim, Prec> const& _left, mat<Dim, Prec> const& _right) -> mat<Dim, Prec> {

	static_assert(Dim == 3 || Dim == 4, "Unsupported matrix order for multiplication");

	auto result = mat<Dim, Prec>();

	if constexpr (Dim == 3) {
		result[0] = _left[0]*_right[0][0] + _left[1]*_right[0][1] + _left[2]*_right[0][2];
		result[1] = _left[0]*_right[1][0] + _left[1]*_right[1][1] + _left[2]*_right[1][2];
		result[2] = _left[0]*_right[2][0] + _left[1]*_right[2][1] + _left[2]*_right[2][2];
	} else if constexpr (Dim == 4) {
		result[0] = _left[0]*_right[0][0] + _left[1]*_right[0][1] + _left[2]*_right[0][2] + _left[3]*_right[0][3];
		result[1] = _left[0]*_right[1][0] + _left[1]*_right[1][1] + _left[2]*_right[1][2] + _left[3]*_right[1][3];
		result[2] = _left[0]*_right[2][0] + _left[1]*_right[2][1] + _left[2]*_right[2][2] + _left[3]*_right[2][3];
		result[3] = _left[0]*_right[3][0] + _left[1]*_right[3][1] + _left[2]*_right[3][2] + _left[3]*_right[3][3];
	}
	return result;

}

template<usize Dim, std::floating_point Prec>
constexpr auto operator*(mat<Dim, Prec> const &_left, vec<Dim, Prec> const &_right) -> vec<Dim, Prec> {

	auto result = vec<Dim, Prec>();

	auto leftT = transpose(_left);
	for (auto i: stx::iota(0_zu, Dim))
		result[i] = dot(leftT[i], _right);
	return result;

}

template<usize Dim, std::floating_point Prec>
constexpr auto operator*(mat<Dim, Prec> const &_left, Prec _right) -> mat<Dim, Prec> {

	auto result = _left;
	result *= _right;
	return result;

}

template<usize Dim, std::floating_point Prec>
constexpr auto operator/(mat<Dim, Prec> const &_left, Prec _right) -> mat<Dim, Prec> {

	auto result = _left;
	result /= _right;
	return result;

}

template<usize Dim, std::floating_point Prec>
constexpr auto transpose(mat<Dim, Prec> const &_mat) -> mat<Dim, Prec> {

	auto result = mat<Dim, Prec>();

	for (auto x: stx::iota(0_zu, Dim))
		for (auto y: stx::iota(0_zu, Dim))
			result[x][y] = _mat[y][x];
	return result;

}

template<usize Dim, std::floating_point Prec>
constexpr auto inverse(mat<Dim, Prec> const &_mat) -> mat<Dim, Prec> {

	static_assert(Dim == 3 || Dim == 4, "Unsupported matrix order for inversion");

	if constexpr (Dim == 3) {

		auto oneOverDeterminant = Prec(1) / (
			+ _mat[0][0] * (_mat[1][1] * _mat[2][2] - _mat[2][1] * _mat[1][2])
			- _mat[1][0] * (_mat[0][1] * _mat[2][2] - _mat[2][1] * _mat[0][2])
			+ _mat[2][0] * (_mat[0][1] * _mat[1][2] - _mat[1][1] * _mat[0][2]));

		auto result = mat<3, Prec>();
		result[0][0] = +(_mat[1][1] * _mat[2][2] - _mat[2][1] * _mat[1][2]) * oneOverDeterminant;
		result[1][0] = -(_mat[1][0] * _mat[2][2] - _mat[2][0] * _mat[1][2]) * oneOverDeterminant;
		result[2][0] = +(_mat[1][0] * _mat[2][1] - _mat[2][0] * _mat[1][1]) * oneOverDeterminant;
		result[0][1] = -(_mat[0][1] * _mat[2][2] - _mat[2][1] * _mat[0][2]) * oneOverDeterminant;
		result[1][1] = +(_mat[0][0] * _mat[2][2] - _mat[2][0] * _mat[0][2]) * oneOverDeterminant;
		result[2][1] = -(_mat[0][0] * _mat[2][1] - _mat[2][0] * _mat[0][1]) * oneOverDeterminant;
		result[0][2] = +(_mat[0][1] * _mat[1][2] - _mat[1][1] * _mat[0][2]) * oneOverDeterminant;
		result[1][2] = -(_mat[0][0] * _mat[1][2] - _mat[1][0] * _mat[0][2]) * oneOverDeterminant;
		result[2][2] = +(_mat[0][0] * _mat[1][1] - _mat[1][0] * _mat[0][1]) * oneOverDeterminant;
		return result;

	} else if constexpr (Dim == 4) {

		auto coef00 = _mat[2][2] * _mat[3][3] - _mat[3][2] * _mat[2][3];
		auto coef02 = _mat[1][2] * _mat[3][3] - _mat[3][2] * _mat[1][3];
		auto coef03 = _mat[1][2] * _mat[2][3] - _mat[2][2] * _mat[1][3];

		auto coef04 = _mat[2][1] * _mat[3][3] - _mat[3][1] * _mat[2][3];
		auto coef06 = _mat[1][1] * _mat[3][3] - _mat[3][1] * _mat[1][3];
		auto coef07 = _mat[1][1] * _mat[2][3] - _mat[2][1] * _mat[1][3];

		auto coef08 = _mat[2][1] * _mat[3][2] - _mat[3][1] * _mat[2][2];
		auto coef10 = _mat[1][1] * _mat[3][2] - _mat[3][1] * _mat[1][2];
		auto coef11 = _mat[1][1] * _mat[2][2] - _mat[2][1] * _mat[1][2];

		auto coef12 = _mat[2][0] * _mat[3][3] - _mat[3][0] * _mat[2][3];
		auto coef14 = _mat[1][0] * _mat[3][3] - _mat[3][0] * _mat[1][3];
		auto coef15 = _mat[1][0] * _mat[2][3] - _mat[2][0] * _mat[1][3];

		auto coef16 = _mat[2][0] * _mat[3][2] - _mat[3][0] * _mat[2][2];
		auto coef18 = _mat[1][0] * _mat[3][2] - _mat[3][0] * _mat[1][2];
		auto coef19 = _mat[1][0] * _mat[2][2] - _mat[2][0] * _mat[1][2];

		auto coef20 = _mat[2][0] * _mat[3][1] - _mat[3][0] * _mat[2][1];
		auto coef22 = _mat[1][0] * _mat[3][1] - _mat[3][0] * _mat[1][1];
		auto coef23 = _mat[1][0] * _mat[2][1] - _mat[2][0] * _mat[1][1];

		auto fac0 = vec<4, Prec>{coef00, coef00, coef02, coef03};
		auto fac1 = vec<4, Prec>{coef04, coef04, coef06, coef07};
		auto fac2 = vec<4, Prec>{coef08, coef08, coef10, coef11};
		auto fac3 = vec<4, Prec>{coef12, coef12, coef14, coef15};
		auto fac4 = vec<4, Prec>{coef16, coef16, coef18, coef19};
		auto fac5 = vec<4, Prec>{coef20, coef20, coef22, coef23};

		auto v0 = vec<4, Prec>{_mat[1][0], _mat[0][0], _mat[0][0], _mat[0][0]};
		auto v1 = vec<4, Prec>{_mat[1][1], _mat[0][1], _mat[0][1], _mat[0][1]};
		auto v2 = vec<4, Prec>{_mat[1][2], _mat[0][2], _mat[0][2], _mat[0][2]};
		auto v3 = vec<4, Prec>{_mat[1][3], _mat[0][3], _mat[0][3], _mat[0][3]};

		auto inv0 = vec<4, Prec>{v1 * fac0 - v2 * fac1 + v3 * fac2};
		auto inv1 = vec<4, Prec>{v0 * fac0 - v2 * fac3 + v3 * fac4};
		auto inv2 = vec<4, Prec>{v0 * fac1 - v1 * fac3 + v3 * fac5};
		auto inv3 = vec<4, Prec>{v0 * fac2 - v1 * fac4 + v2 * fac5};

		auto signA = vec<4, Prec>{+1, -1, +1, -1};
		auto signB = vec<4, Prec>{-1, +1, -1, +1};
		auto inv = mat<4, Prec>{inv0 * signA, inv1 * signB, inv2 * signA, inv3 * signB};

		auto row0 = vec<4, Prec>{inv[0][0], inv[1][0], inv[2][0], inv[3][0]};

		auto dot0 = _mat[0] * row0;
		auto dot1 = (dot0.x() + dot0.y()) + (dot0.z() + dot0.w());

		auto oneOverDeterminant = Prec(1) / dot1;

		return inv * oneOverDeterminant;

	}

}

template<std::floating_point Prec>
constexpr auto look(vec<3, Prec> _pos, vec<3, Prec> _dir, vec<3, Prec> _up) -> mat<4, Prec> {

	ASSUME(isUnit(_dir));
	ASSUME(isUnit(_up));

	auto result = mat<4, Prec>::identity();

	auto s = normalize(cross(_dir, _up));
	auto u = cross(s, _dir);
	result[0][0] = s[0];
	result[1][0] = s[1];
	result[2][0] = s[2];
	result[0][1] = u[0];
	result[1][1] = u[1];
	result[2][1] = u[2];
	result[0][2] = -_dir[0];
	result[1][2] = -_dir[1];
	result[2][2] = -_dir[2];
	result[3][0] = -dot(s, _pos);
	result[3][1] = -dot(u, _pos);
	result[3][2] = dot(_dir, _pos);
	return result;

}

template<std::floating_point Prec>
constexpr auto perspective(Prec _vFov, Prec _aspectRatio, Prec _zNear) -> mat<4, Prec> {

	auto h = Prec(1) / tan(Prec(0.5) * _vFov);
	auto w = h * _aspectRatio;

	auto result = mat<4, Prec>();
	result.fill(0);
	result[0][0] = w;
	result[1][1] = h;
	result[2][3] = Prec(1);
	result[3][2] = _zNear;
	return result;

}

}
