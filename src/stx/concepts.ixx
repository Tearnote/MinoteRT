export module minote.concepts;

import <type_traits>;

// Any enum or enum struct
export template<typename T>
concept scoped_enum = std::is_scoped_enum_v<T>;

// Built-in type with defined arithmetic operations (+, -, *, /)
export template<typename T>
concept arithmetic = std::is_arithmetic_v<T>;
