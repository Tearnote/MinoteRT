export module minote.math;

import <initializer_list>;
import <type_traits>;
import <algorithm>;
import <concepts>;
import <numbers>;
import <cassert>;
import <cmath>;
import minote.concepts;
import minote.ranges;
import minote.types;

// Constants

export template<std::floating_point T>
constexpr auto Pi_v = std::numbers::pi_v<T>;
export constexpr auto Pi = Pi_v<float>;

export template<std::floating_point T>
constexpr auto Tau_v = Pi_v<T> *T(2.0);
export constexpr auto Tau = Tau_v<float>;

// Scalar operations

export template<arithmetic T, std::floating_point Prec = float>
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
export template<std::integral T>
constexpr auto tmod(T num, T div) { return num % div + (num % div < 0) * div; }

// GLSL-style scalar clamp
export template<arithmetic T>
constexpr auto clamp(T val, T vmin, T vmax) -> T { return std::max(vmin, std::min(val, vmax)); }

// Compound types

// Generic math vector, of any dimension between 2 to 4 and any underlying type
export template<usize Dim, arithmetic T>
class vec {
public:
	static_assert(Dim >= 2 && Dim <= 4, "Vectors need to have 2, 3 or 4 components");
	using self_t = vec<Dim, T>;

	// Creation

	// Uninitialized init
	constexpr vec() = default;

	// Fill the vector with copies of the value
	explicit constexpr vec(T fillVal) { fill(fillVal); }

	// Create the vector with provided component values
	constexpr vec(std::initializer_list<T> list) {
		std::ranges::copy(list, arr.begin());
	}

	// Conversions

	// Type cast
	template<arithmetic U>
		requires (!std::same_as<T, U>)
	explicit constexpr vec(vec<Dim, U> const& other) {
		for (auto i: iota(0_zu, Dim))
			arr[i] = T(other[i]);
	}

	// Dimension downcast
	template<usize N>
		requires (N > Dim)
	explicit constexpr vec(vec<N, T> const& other) {
		for (auto i: iota(0_zu, Dim))
			arr[i] = other[i];
	}

	// Dimension upcast
	template<usize N>
		requires (N < Dim)
	constexpr vec(vec<N, T> const& other, T fill) {
		arr.fill(fill);
		for (auto i: iota(0_zu, N))
			arr[i] = other[i];
	}

	// Member access

	[[nodiscard]]
	constexpr auto at(usize n) -> T& { return arr[n]; }
	[[nodiscard]]
	constexpr auto at(usize n) const -> T { return arr[n]; }

	[[nodiscard]]
	constexpr auto operator[](usize n) -> T& { return at(n); }
	[[nodiscard]]
	constexpr auto operator[](usize n) const -> T { return at(n); }

	[[nodiscard]]
	constexpr auto x() -> T& { static_assert(Dim >= 1); return arr[0]; }
	[[nodiscard]]
	constexpr auto x() const -> T { static_assert(Dim >= 1); return arr[0]; }
	[[nodiscard]]
	constexpr auto y() -> T& { static_assert(Dim >= 2); return arr[1]; }
	[[nodiscard]]
	constexpr auto y() const -> T { static_assert(Dim >= 2); return arr[1]; }
	[[nodiscard]]
	constexpr auto z() -> T& { static_assert(Dim >= 3); return arr[2]; }
	[[nodiscard]]
	constexpr auto z() const -> T { static_assert(Dim >= 3); return arr[2]; }
	[[nodiscard]]
	constexpr auto w() -> T& { static_assert(Dim >= 4); return arr[3]; }
	[[nodiscard]]
	constexpr auto w() const -> T { static_assert(Dim >= 4); return arr[3]; }

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

	constexpr void fill(T val) { arr.fill(val); }

	// Vector operations

	// Component-wise arithmetic

	constexpr auto operator+=(self_t const& other) -> self_t& {
		for (auto i: iota(0_zu, Dim))
			arr[i] += other[i];
		return *this;
	}

	constexpr auto operator-=(self_t const& other) -> self_t& {
		for (auto i: iota(0_zu, Dim))
			arr[i] -= other[i];
		return *this;
	}

	constexpr auto operator*=(self_t const& other) -> self_t& {
		for (auto i: iota(0_zu, Dim))
			arr[i] *= other[i];
		return *this;
	}

	constexpr auto operator/=(self_t const& other) -> self_t& {
		for (auto i: iota(0_zu, Dim))
			arr[i] /= other[i];
		return *this;
	}

	constexpr auto operator%=(self_t const& other) -> self_t& {
		static_assert(std::is_integral_v<T>);

		for (auto i: iota(0_zu, Dim))
			arr[i] %= other[i];
		return *this;
	}

	// Scalar arithmetic

	constexpr auto operator*=(T other) -> self_t& {
		for (auto i: iota(0_zu, Dim))
			arr[i] *= other;
		return *this;
	}

	constexpr auto operator/=(T other) -> self_t& {
		for (auto i: iota(0_zu, Dim))
			arr[i] /= other;
		return *this;
	}

	constexpr auto operator%=(T other) -> self_t& {
		static_assert(std::is_integral_v<T>);

		for (auto i: iota(0_zu, Dim))
			arr[i] %= other;
		return *this;
	}

	constexpr auto operator<<=(T other) -> self_t& {
		static_assert(std::is_integral_v<T>);

		for (auto i: iota(0_zu, Dim))
			arr[i] <<= other;
		return *this;
	}

	constexpr auto operator>>=(T other) -> self_t& {
		static_assert(std::is_integral_v<T>);

		for (auto i: iota(0_zu, Dim))
			arr[i] >>= other;
		return *this;
	}

private:
	std::array<T, Dim> arr;
};

// Binary vector operations

export template<usize Dim, arithmetic T>
constexpr auto operator+(vec<Dim, T> const& left, vec<Dim, T> const& right) -> vec<Dim, T> {
	auto result = left;
	result += right;
	return result;
}

export template<usize Dim, arithmetic T>
constexpr auto operator-(vec<Dim, T> const& left, vec<Dim, T> const& right) -> vec<Dim, T> {
	auto result = left;
	result -= right;
	return result;
}

export template<usize Dim, arithmetic T>
constexpr auto operator*(vec<Dim, T> const& left, vec<Dim, T> const& right) -> vec<Dim, T> {
	auto result = left;
	result *= right;
	return result;
}

export template<usize Dim, arithmetic T>
constexpr auto operator/(vec<Dim, T> const& left, vec<Dim, T> const& right) -> vec<Dim, T> {
	auto result = left;
	result /= right;
	return result;
}

export template<usize Dim, std::integral T>
constexpr auto operator%(vec<Dim, T> const& left, vec<Dim, T> const& right) -> vec<Dim, T> {
	auto result = left;
	result %= right;
	return result;
}

export template<usize Dim, arithmetic T>
constexpr auto operator==(vec<Dim, T> const& left, vec<Dim, T> const& right) -> bool {
	for (auto i: iota(0_zu, Dim))
		if (left[i] != right[i])
			return false;
	return true;
}

export template<usize Dim, arithmetic T>
constexpr auto min(vec<Dim, T> const& left, vec<Dim, T> const& right) -> vec<Dim, T> {
	auto result = vec<Dim, T>();
	for (auto i: iota(0_zu, Dim))
		result[i] = std::min(left[i], right[i]);
	return result;
}

export template<usize Dim, arithmetic T>
constexpr auto max(vec<Dim, T> const& left, vec<Dim, T> const& right) -> vec<Dim, T> {
	auto result = vec<Dim, T>();
	for (auto i: iota(0_zu, Dim))
		result[i] = std::max(left[i], right[i]);
	return result;
}

export template<usize Dim, arithmetic T>
constexpr auto dot(vec<Dim, T> const& left, vec<Dim, T> const& right) -> T {
	auto result = T(0);
	for (auto i: iota(0_zu, Dim))
		result += left[i] * right[i];
	return result;
}

export template<arithmetic T>
constexpr auto cross(vec<3, T> const& left, vec<3, T> const& right) -> vec<3, T> {
	return vec<3, T>{
		left[1] * right[2] - right[1] * left[2],
		left[2] * right[0] - right[2] * left[0],
		left[0] * right[1] - right[0] * left[1],
	};
}

// Binary scalar operations

export template<usize Dim, arithmetic T>
constexpr auto operator*(vec<Dim, T> const& left, T right) -> vec<Dim, T> {
	auto result = left;
	result *= right;
	return result;
}

export template<usize Dim, arithmetic T>
constexpr auto operator*(T left, vec<Dim, T> const& right) -> vec<Dim, T> { return right * left; }

export template<usize Dim, arithmetic T>
constexpr auto operator/(vec<Dim, T> const& left, T right) -> vec<Dim, T> {
	auto result = left;
	result /= right;
	return result;
}

export template<usize Dim, std::integral T>
constexpr auto operator%(vec<Dim, T> const& left, T right) -> vec<Dim, T> {
	auto result = left;
	result %= right;
	return result;
}

export template<usize Dim, std::integral T>
constexpr auto operator<<(vec<Dim, T> const& left, T right) -> vec<Dim, T> {
	auto result = left;
	result <<= right;
	return result;
}

export template<usize Dim, std::integral T>
constexpr auto operator>>(vec<Dim, T> const& left, T right) -> vec<Dim, T> {
	auto result = left;
	result >>= right;
	return result;
}

// Unary vector operations

// Component-wise absolute value
export template<usize Dim, std::floating_point T>
constexpr auto abs(vec<Dim, T> const& v) -> vec<Dim, T> {
	auto result = vec<Dim, T>();
	for (auto i: iota(0_zu, Dim))
		result[i] = abs(v[i]);
	return result;
}

// Vector length as Euclidean distance
export template<usize Dim, std::floating_point T>
constexpr auto length(vec<Dim, T> const& v) -> T { return sqrt(length2(v)); }

// Square of vector length (faster to compute than length)
export template<usize Dim, arithmetic T>
constexpr auto length2(vec<Dim, T> const& v) -> T { return dot(v, v); }

// true if vector has the length of 1 (within reasonable epsilon)
export template<usize Dim, std::floating_point T>
constexpr auto isUnit(vec<Dim, T> const& v) -> bool { return (abs(length2(v) - 1) < (1.0 / 16.0)); }

// Constructs a vector in the same direction but length 1
export template<usize Dim, std::floating_point T>
constexpr auto normalize(vec<Dim, T> const& v) -> vec<Dim, T> {
	if constexpr (Dim == 4) {
		auto norm = normalize(vec<3, T>(v));
		return vec<Dim, T>(norm, v[3]);
	}
	return v / length(v);
}

//=== GLSL-like vector aliases

export using vec2 = vec<2, float>;
export using vec3 = vec<3, float>;
export using vec4 = vec<4, float>;
export using ivec2 = vec<2, int>;
export using ivec3 = vec<3, int>;
export using ivec4 = vec<4, int>;
export using uvec2 = vec<2, uint>;
export using uvec3 = vec<3, uint>;
export using uvec4 = vec<4, uint>;
export using u8vec2 = vec<2, uint8>;
export using u8vec3 = vec<3, uint8>;
export using u8vec4 = vec<4, uint8>;
export using u16vec2 = vec<2, uint16>;
export using u16vec3 = vec<3, uint16>;
export using u16vec4 = vec<4, uint16>;

#ifndef __INTELLISENSE__
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
#endif //__INTELLISENSE__

// Quaternion, equivalent to a vec4 but with unique operations available.
// Main purpose is representing rotations. Data layout is {w, x, y, z}.
export template<std::floating_point Prec = float>
class qua {
public:
	// Creation

	// Uninitialized init
	constexpr qua() = default;

	// Create the quaternion with provided {w, x, y, z} values
	constexpr qua(std::initializer_list<Prec> list) {
		std::ranges::copy(list, arr.begin());
	}

	// Convert a position vector into a quaternion
	template<usize N>
		requires (N == 3 || N == 4)
	constexpr explicit qua(vec<N, Prec> const& other) {
		arr[0] = 0;
		arr[1] = other.x();
		arr[2] = other.y();
		arr[3] = other.z();
	}

	// Create a unit quaternion that represents no rotation
	static constexpr auto identity() -> qua<Prec> { return qua<Prec>{1, 0, 0, 0}; }

	// Create a unit quaternion that represents a rotation around an arbitrary axis
	static constexpr auto angleAxis(Prec angle, vec<3, Prec> axis) -> qua<Prec> {
		assert(isUnit(axis));
		auto halfAngle = angle / Prec(2);
		auto sinHalfAngle = std::sin(halfAngle);
		return qua<Prec>{
			std::cos(halfAngle),
			sinHalfAngle * axis[0],
			sinHalfAngle * axis[1],
			sinHalfAngle * axis[2],
		};
	}

	// Conversion

	// Type cast
	template<arithmetic U>
		requires (!std::same_as<Prec, U>)
	explicit constexpr qua(qua<U> const& other) {
		for (auto i: iota(0_zu, arr.size()))
			arr[i] = other[i];
	}

	// Member access

	constexpr auto at(usize n) -> Prec& { return arr[n]; }
	constexpr auto at(usize n) const -> Prec { return arr[n]; }

	constexpr auto operator[](usize n) -> Prec& { return at(n); }
	constexpr auto operator[](usize n) const -> Prec { return at(n); }

	constexpr auto w() -> Prec& { return arr[0]; }
	constexpr auto w() const -> Prec { return arr[0]; }
	constexpr auto x() -> Prec& { return arr[1]; }
	constexpr auto x() const -> Prec { return arr[1]; }
	constexpr auto y() -> Prec& { return arr[2]; }
	constexpr auto y() const -> Prec { return arr[2]; }
	constexpr auto z() -> Prec& { return arr[3]; }
	constexpr auto z() const -> Prec { return arr[3]; }

private:
	std::array<Prec, 4> arr;
};

// Binary quaternion operations

export template<std::floating_point Prec>
constexpr auto operator*(qua<Prec> const& left, qua<Prec> const& right) -> qua<Prec> {
	return qua<Prec>{
		-left.x() * right.x() - left.y() * right.y() - left.z() * right.z() + left.w() * right.w(),
		 left.x() * right.w() + left.y() * right.z() - left.z() * right.y() + left.w() * right.x(),
		-left.x() * right.z() + left.y() * right.w() + left.z() * right.x() + left.w() * right.y(),
		 left.x() * right.y() - left.y() * right.x() + left.z() * right.w() + left.w() * right.z(),
	};
}

// Quaternion alias

export using quat = qua<float>;

// Generic matrix type, of order 3 or 4, and any floating-point precision
export template<usize Dim, std::floating_point Prec>
class mat {
public:
	using col_t = vec<Dim, Prec>;

	static_assert(Dim >= 3 && Dim <= 4, "Matrices need to have order 3x3 or 4x4");

	// Creation

	// Uninitialized init
	constexpr mat() = default;

	// Compose a matrix out of all component values, in column-major order
	constexpr mat(std::initializer_list<Prec> list) {
		auto it = list.begin();
		for (auto x: iota(0_zu, Dim)) {
			for (auto y: iota(0_zu, Dim)) {
				arr[x][y] = *it;
				it += 1;
			}
		}
	}

	// Compose a matrix out of column vectors
	constexpr mat(std::initializer_list<col_t> list) {
		std::ranges::copy(list, arr.begin());
	}

	// Create a matrix that is a no-op on multiplication
	static constexpr auto identity() -> mat<Dim, Prec> {
		auto result = mat<Dim, Prec>();
		result.fill(0);
		for (auto i: iota(0_zu, Dim))
			result[i][i] = 1;
		return result;
	}

	// Classic translation, rotation and scale matrices for vector manipulation

	static constexpr auto translate(vec<3, Prec> shift) -> mat<Dim, Prec> {
		auto result = mat<Dim, Prec>::identity();
		result[3][0] = shift[0];
		result[3][1] = shift[1];
		result[3][2] = shift[2];
		return result;
	}

	static constexpr auto rotate(vec<3, Prec> axis, Prec angle) -> mat<Dim, Prec> {
		assert(isUnit(axis));

		auto sinT = std::sin(angle);
		auto cosT = std::cos(angle);
		auto temp = axis * (Prec(1) - cosT);

		auto result = mat<Dim, Prec>::identity();

		result[0][0] = cosT + temp[0] * axis[0];
		result[0][1] = temp[0] * axis[1] + sinT * axis[2];
		result[0][2] = temp[0] * axis[2] - sinT * axis[1];

		result[1][0] = temp[1] * axis[0] - sinT * axis[2];
		result[1][1] = cosT + temp[1] * axis[1];
		result[1][2] = temp[1] * axis[2] + sinT * axis[0];

		result[2][0] = temp[2] * axis[0] + sinT * axis[1];
		result[2][1] = temp[2] * axis[1] - sinT * axis[0];
		result[2][2] = cosT + temp[2] * axis[2];

		return result;
	}

	static constexpr auto rotate(qua<Prec> quat) -> mat<Dim, Prec> {
		auto result = mat<Dim, Prec>::identity();

		result[0][0] = 1.0f - 2.0f * (quat.y() * quat.y() + quat.z() * quat.z());
		result[0][1] =        2.0f * (quat.x() * quat.y() + quat.z() * quat.w());
		result[0][2] =        2.0f * (quat.x() * quat.z() - quat.y() * quat.w());

		result[1][0] =        2.0f * (quat.x() * quat.y() - quat.z() * quat.w());
		result[1][1] = 1.0f - 2.0f * (quat.x() * quat.x() + quat.z() * quat.z());
		result[1][2] =        2.0f * (quat.y() * quat.z() + quat.x() * quat.w());

		result[2][0] =        2.0f * (quat.x() * quat.z() + quat.y() * quat.w());
		result[2][1] =        2.0f * (quat.y() * quat.z() - quat.x() * quat.w());
		result[2][2] = 1.0f - 2.0f * (quat.x() * quat.x() + quat.y() * quat.y());

		return result;
	}

	static constexpr auto scale(vec<3, Prec> scale) -> mat<Dim, Prec> {
		auto result = mat<Dim, Prec>::identity();
		for (auto i: iota(0_zu, 3_zu))
			result[i][i] = scale[i];
		return result;
	}

	static constexpr auto scale(Prec scale) -> mat<Dim, Prec> {
		auto result = mat<Dim, Prec>::identity();
		for (auto i: iota(0_zu, 3_zu))
			result[i][i] = scale;
		return result;
	}

	// Conversion

	// Type cast
	template<arithmetic U>
		requires (!std::same_as<Prec, U>)
	explicit constexpr mat(mat<Dim, U> const& other) {
		for (auto x: iota(0_zu, Dim))
			for (auto y: iota(0_zu, Dim))
				arr[x][y] = other[x][y];
	}

	// Dimension cast
	template<usize N>
		requires (N != Dim)
	explicit constexpr mat(mat<N, Prec> const& other) {
		if constexpr (Dim > N)
			*this = identity();

		auto const Smaller = std::min(Dim, N);
		for (auto x: iota(0_zu, Smaller))
			for (auto y: iota(0_zu, Smaller))
				arr[x][y] = other[x][y];
	}

	// Member access

	constexpr auto at(usize x, usize y) -> Prec& { return arr[x][y]; }
	constexpr auto at(usize x, usize y) const -> Prec { return arr[x][y]; }

	constexpr auto operator[](usize x) -> col_t& { return arr[x]; }
	constexpr auto operator[](usize x) const -> col_t const& { return arr[x]; }

	constexpr auto fill(Prec val) { for (auto& col : arr) col.fill(val); }

	// Operations

	// Scalar arithmetic

	constexpr auto operator*=(Prec other) -> mat<Dim, Prec>& {
		for (auto x: iota(0_zu, Dim))
			for (auto y: iota(0_zu, Dim))
				arr[x][y] *= other;
		return *this;
	}

	constexpr auto operator/=(Prec other) -> mat<Dim, Prec>& {
		for (auto x: iota(0_zu, Dim))
			for (auto y: iota(0_zu, Dim))
				arr[x][y] /= other;
		return *this;
	}

private:
	std::array<col_t, Dim> arr;
};

// Binary matrix operations

export template<usize Dim, std::floating_point Prec>
constexpr auto operator*(mat<Dim, Prec> const& left, mat<Dim, Prec> const& right) -> mat<Dim, Prec> {
	static_assert(Dim == 3 || Dim == 4, "Unsupported matrix order for multiplication");

	auto result = mat<Dim, Prec>();

	if constexpr (Dim == 3) {
		result[0] = left[0] * right[0][0] + left[1] * right[0][1] + left[2] * right[0][2];
		result[1] = left[0] * right[1][0] + left[1] * right[1][1] + left[2] * right[1][2];
		result[2] = left[0] * right[2][0] + left[1] * right[2][1] + left[2] * right[2][2];
	} else if constexpr (Dim == 4) {
		result[0] = left[0] * right[0][0] + left[1] * right[0][1] + left[2] * right[0][2] + left[3] * right[0][3];
		result[1] = left[0] * right[1][0] + left[1] * right[1][1] + left[2] * right[1][2] + left[3] * right[1][3];
		result[2] = left[0] * right[2][0] + left[1] * right[2][1] + left[2] * right[2][2] + left[3] * right[2][3];
		result[3] = left[0] * right[3][0] + left[1] * right[3][1] + left[2] * right[3][2] + left[3] * right[3][3];
	}
	return result;
}

export template<usize Dim, std::floating_point Prec>
constexpr auto operator*(mat<Dim, Prec> const& left, vec<Dim, Prec> const& right) -> vec<Dim, Prec> {
	auto result = vec<Dim, Prec>();

	auto leftT = transpose(left);
	for (auto i: iota(0_zu, Dim))
		result[i] = dot(leftT[i], right);
	return result;
}

export template<usize Dim, std::floating_point Prec>
constexpr auto operator*(mat<Dim, Prec> const& left, Prec right) -> mat<Dim, Prec> {
	auto result = left;
	result *= right;
	return result;
}

export template<usize Dim, std::floating_point Prec>
constexpr auto operator*(Prec left, mat<Dim, Prec> const& right) -> mat<Dim, Prec> { return right * left; }

export template<usize Dim, std::floating_point Prec>
constexpr auto operator/(mat<Dim, Prec> const& left, Prec right) -> mat<Dim, Prec> {
	auto result = left;
	result /= right;
	return result;
}

// Unary matrix operations

// Creates a matrix with rows transposed with columns
export template<usize Dim, std::floating_point Prec>
constexpr auto transpose(mat<Dim, Prec> const& m) -> mat<Dim, Prec> {
	auto result = mat<Dim, Prec>();

	for (auto x: iota(0_zu, Dim))
		for (auto y: iota(0_zu, Dim))
			result[x][y] = m[y][x];
	return result;
}

// Creates a matrix that results in identity when multiplied with the original (slow!)
export template<usize Dim, std::floating_point Prec>
constexpr auto inverse(mat<Dim, Prec> const& m) -> mat<Dim, Prec> {
	static_assert(Dim == 3 || Dim == 4, "Unsupported matrix order for inversion");

	if constexpr (Dim == 3) {
		auto oneOverDeterminant = Prec(1) / (
			  m[0][0] * (m[1][1] * m[2][2] - m[2][1] * m[1][2])
			- m[1][0] * (m[0][1] * m[2][2] - m[2][1] * m[0][2])
			+ m[2][0] * (m[0][1] * m[1][2] - m[1][1] * m[0][2])
		);

		auto result = mat<3, Prec>();
		result[0][0] =  (m[1][1] * m[2][2] - m[2][1] * m[1][2]) * oneOverDeterminant;
		result[1][0] = -(m[1][0] * m[2][2] - m[2][0] * m[1][2]) * oneOverDeterminant;
		result[2][0] =  (m[1][0] * m[2][1] - m[2][0] * m[1][1]) * oneOverDeterminant;
		result[0][1] = -(m[0][1] * m[2][2] - m[2][1] * m[0][2]) * oneOverDeterminant;
		result[1][1] =  (m[0][0] * m[2][2] - m[2][0] * m[0][2]) * oneOverDeterminant;
		result[2][1] = -(m[0][0] * m[2][1] - m[2][0] * m[0][1]) * oneOverDeterminant;
		result[0][2] =  (m[0][1] * m[1][2] - m[1][1] * m[0][2]) * oneOverDeterminant;
		result[1][2] = -(m[0][0] * m[1][2] - m[1][0] * m[0][2]) * oneOverDeterminant;
		result[2][2] =  (m[0][0] * m[1][1] - m[1][0] * m[0][1]) * oneOverDeterminant;
		return result;
	} else if constexpr (Dim == 4) {
		auto coef00 = m[2][2] * m[3][3] - m[3][2] * m[2][3];
		auto coef02 = m[1][2] * m[3][3] - m[3][2] * m[1][3];
		auto coef03 = m[1][2] * m[2][3] - m[2][2] * m[1][3];

		auto coef04 = m[2][1] * m[3][3] - m[3][1] * m[2][3];
		auto coef06 = m[1][1] * m[3][3] - m[3][1] * m[1][3];
		auto coef07 = m[1][1] * m[2][3] - m[2][1] * m[1][3];

		auto coef08 = m[2][1] * m[3][2] - m[3][1] * m[2][2];
		auto coef10 = m[1][1] * m[3][2] - m[3][1] * m[1][2];
		auto coef11 = m[1][1] * m[2][2] - m[2][1] * m[1][2];

		auto coef12 = m[2][0] * m[3][3] - m[3][0] * m[2][3];
		auto coef14 = m[1][0] * m[3][3] - m[3][0] * m[1][3];
		auto coef15 = m[1][0] * m[2][3] - m[2][0] * m[1][3];

		auto coef16 = m[2][0] * m[3][2] - m[3][0] * m[2][2];
		auto coef18 = m[1][0] * m[3][2] - m[3][0] * m[1][2];
		auto coef19 = m[1][0] * m[2][2] - m[2][0] * m[1][2];

		auto coef20 = m[2][0] * m[3][1] - m[3][0] * m[2][1];
		auto coef22 = m[1][0] * m[3][1] - m[3][0] * m[1][1];
		auto coef23 = m[1][0] * m[2][1] - m[2][0] * m[1][1];

		auto fac0 = vec<4, Prec>{coef00, coef00, coef02, coef03};
		auto fac1 = vec<4, Prec>{coef04, coef04, coef06, coef07};
		auto fac2 = vec<4, Prec>{coef08, coef08, coef10, coef11};
		auto fac3 = vec<4, Prec>{coef12, coef12, coef14, coef15};
		auto fac4 = vec<4, Prec>{coef16, coef16, coef18, coef19};
		auto fac5 = vec<4, Prec>{coef20, coef20, coef22, coef23};

		auto v0 = vec<4, Prec>{m[1][0], m[0][0], m[0][0], m[0][0]};
		auto v1 = vec<4, Prec>{m[1][1], m[0][1], m[0][1], m[0][1]};
		auto v2 = vec<4, Prec>{m[1][2], m[0][2], m[0][2], m[0][2]};
		auto v3 = vec<4, Prec>{m[1][3], m[0][3], m[0][3], m[0][3]};

		auto inv0 = vec<4, Prec>{v1 * fac0 - v2 * fac1 + v3 * fac2};
		auto inv1 = vec<4, Prec>{v0 * fac0 - v2 * fac3 + v3 * fac4};
		auto inv2 = vec<4, Prec>{v0 * fac1 - v1 * fac3 + v3 * fac5};
		auto inv3 = vec<4, Prec>{v0 * fac2 - v1 * fac4 + v2 * fac5};

		auto signA = vec<4, Prec>{ 1, -1,  1, -1};
		auto signB = vec<4, Prec>{-1,  1, -1,  1};
		auto inv = mat<4, Prec>{inv0 * signA, inv1 * signB, inv2 * signA, inv3 * signB};

		auto row0 = vec<4, Prec>{inv[0][0], inv[1][0], inv[2][0], inv[3][0]};

		auto dot0 = m[0] * row0;
		auto dot1 = (dot0.x() + dot0.y()) + (dot0.z() + dot0.w());

		auto oneOverDeterminant = Prec(1) / dot1;

		return inv * oneOverDeterminant;
	}
}

// Specialized matrix generators

// Variant of lookAt matrix. Dir is a unit vector of the camera direction.
// Dir and Up are both required to be unit vectors
export template<std::floating_point Prec = float>
constexpr auto look(vec<3, Prec> pos, vec<3, Prec> dir, vec<3, Prec> up) -> mat<4, Prec> {
	assert(isUnit(dir));
	assert(isUnit(up));

	auto result = mat<4, Prec>::identity();

	auto s = normalize(cross(up, dir));
	auto u = cross(dir, s);
	result[0][0] = -s[0];
	result[1][0] = -s[1];
	result[2][0] = -s[2];
	result[0][1] = u[0];
	result[1][1] = u[1];
	result[2][1] = u[2];
	result[0][2] = dir[0];
	result[1][2] = dir[1];
	result[2][2] = dir[2];
	result[3][0] = dot(s, pos);
	result[3][1] = -dot(u, pos);
	result[3][2] = -dot(dir, pos);
	return result;
}

// Creates a perspective matrix. The matrix uses inverted infinite depth:
// 1.0 at zNear, 0.0 at infinity.
export template<std::floating_point Prec = float>
constexpr auto perspective(Prec vFov, Prec aspectRatio, Prec zNear) -> mat<4, Prec> {
	auto h = Prec(1) / std::tan(Prec(0.5) * vFov);
	auto w = h * aspectRatio;

	auto result = mat<4, Prec>();
	result.fill(0);
	result[0][0] = w;
	result[1][1] = h;
	result[2][3] = Prec(1);
	result[3][2] = zNear;
	return result;
}

// GLSL-like matrix aliases

export using mat3 = mat<3, float>;
export using mat4 = mat<4, float>;

static_assert(std::is_trivially_constructible_v<mat3>);
static_assert(std::is_trivially_constructible_v<mat4>);

// Conversion literals

export consteval auto operator ""_cm(unsigned long long int val) -> float { return double(val) * 0.000'001; }

export consteval auto operator ""_cm(long double val) -> float { return double(val) * 0.000'001; }

export consteval auto operator ""_m(unsigned long long int val) -> float { return double(val) * 0.001; }

export consteval auto operator ""_m(long double val) -> float { return double(val) * 0.001; }

export consteval auto operator ""_km(unsigned long long int val) -> float { return val; }

export consteval auto operator ""_km(long double val) -> float { return val; }

export consteval auto operator ""_deg(unsigned long long int val) -> float { return radians(double(val)); }

export consteval auto operator ""_deg(long double val) -> float { return radians(val); }
