#pragma once

#include <type_traits>

namespace minote::stx {

namespace enum_literals {

template<typename T>
requires std::is_scoped_enum_v<T>
constexpr auto operator+(T e) { return std::to_underlying(e); }

}

}
