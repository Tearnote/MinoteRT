#pragma once

#include <type_traits>

namespace minote::stx {

namespace enum_literals {

template<typename T>
requires std::is_scoped_enum_v<T>
constexpr auto operator+(T e) { return std::underlying_type_t<T>(e); }

}

}
