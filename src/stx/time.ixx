export module minote.time;

import <concepts>;
import minote.concepts;
import minote.types;

// Main timestamp/duration type. Has enough resolution to largely ignore
// rounding error, and wraps after >100 years.
export using nsec = int64;

// Create nsec from a count of seconds
export template<arithmetic T>
constexpr auto seconds(T val) -> nsec { return val * 1'000'000'000LL; }

// Create nsec from a count of milliseconds
export template<arithmetic T>
constexpr auto milliseconds(T val) { return val * 1'000'000LL; }

// Get an accurate floating-point ratio between two nsecs
export template<std::floating_point T = float>
constexpr auto ratio(nsec left, nsec right) -> T { return double(left) / double(right); }

// Create nsec from second/millisecond literals

export constexpr auto operator""_s(unsigned long long val) { return seconds(val); }
export constexpr auto operator""_s(long double val) { return seconds(val); }
export constexpr auto operator""_ms(unsigned long long val) { return milliseconds(val); }
export constexpr auto operator""_ms(long double val) { return milliseconds(val); }
