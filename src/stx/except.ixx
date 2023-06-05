export module minote.except;

import <stdexcept>;
import <utility>;
import <format>;

export template<typename Err, typename... Args>
inline auto typed_error_fmt(std::format_string<Args...> fmt, Args&&... args) -> Err {
	return Err(std::format(fmt, std::forward<Args>(args)...));
}

export template<typename... Args>
inline auto runtime_error_fmt(std::format_string<Args...> fmt, Args&&... args) {
	return typed_error_fmt<std::runtime_error>(fmt, std::forward<Args>(args)...);
};

export template<typename... Args>
inline auto logic_error_fmt(std::format_string<Args...> fmt, Args&&... args) {
	return typed_error_fmt<std::logic_error>(fmt, std::forward<Args>(args)...);
};
