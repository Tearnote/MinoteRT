#pragma once

#include <string_view>
#include <fmtlog.h>
#include "types.hpp"

namespace minote {

// Logging facility. Basic set of features, but threaded and non-blocking - it's
// safe to log even very rapid streams of messages with no performance penalty.
// Features color in console and {fmt} formatting
class Log {

public:

	// Start logging to console and specified logfile. All messages below
	// the provided log level will be dropped
	static void init(std::string_view filename, fmtlog::LogLevel level);

	// Not constructible
	Log() = delete;

private:

	static void consoleCallback(int64, fmtlog::LogLevel, fmt::string_view,
	                            usize, fmt::string_view, fmt::string_view, usize, usize);

};

// Logging functions, fmtlib formatting is supported
#define L_DEBUG(fmt, ...) logd(fmt, ##__VA_ARGS__)
#define L_INFO(fmt, ...) logi(fmt, ##__VA_ARGS__)
#define L_WARN(fmt, ...) logw(fmt, ##__VA_ARGS__)
#define L_ERROR(fmt, ...) loge(fmt, ##__VA_ARGS__)

}