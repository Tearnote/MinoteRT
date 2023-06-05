export module minote.ranges;

import <algorithm>;
import <ranges>;
import <span>;
import minote.types;

// Selective import of ranges library

export using std::ranges::transform;
export using std::ranges::views::iota;
export using std::ranges::views::reverse;

// Safely create a span from pointer+size pair
// Creates a valid empty span if size is 0 or pointer is null
export template<typename T>
auto ptr_span(T* ptr, usize size = 1) {
	if (!ptr || !size)
		return std::span<T>();
	return std::span(ptr, size);
}
