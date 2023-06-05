module;

export module minote.log;

import <string_view>;
import <sstream>;
import <iomanip>;
import <utility>;
import <cstring>;
import <string>;
import <format>;
import <cstdio>;
import <cerrno>;
import <memory>;
import <array>;
import <print>;
import <ctime>;
import minote.service;
import minote.except;

// https://developercommunity.visualstudio.com/t/visual-studio-cant-find-time-function-using-module/1126857
#define WIN32_CTIME_ISSUE

// Simple synchronous logger service
// Logs to both file and console
class Logger_impl {
public:
	enum class Level {
		Debug,
		Info,
		Warn,
		Error,
	};

	template<typename... Args>
	void log(Level level, std::string_view fmt, Args&&... args) {
		auto level_v = std::to_underlying(level);
		if (level_v < std::to_underlying(minLevel)) return;
		auto msg = std::vformat(fmt, std::make_format_args(std::forward<Args>(args)...));

#ifdef WIN32_CTIME_ISSUE
		auto timestamp = _time64(nullptr);
		auto* tm = _localtime64(&timestamp);
#else
		auto timestamp = std::time(nullptr);
		auto* tm = std::localtime(&timestamp);
#endif
		auto timestampStr = std::stringstream();
		timestampStr << std::put_time(tm, "%H:%M:%S");

		auto* confile = level_v >= std::to_underlying(Level::Warn)? stderr : stdout;
		std::print(confile, "{}{} {}{}\n", Colors[level_v], timestampStr.view(), msg, Colors.back());
		std::print(logfile.get(), "{} [{}] {}\n", timestampStr.view(), LevelStr[level_v], msg);

		// Flush after important messages to avoid losing data
		if (level_v >= std::to_underlying(Level::Warn))
			std::fflush(logfile.get());
	}

private:
	constexpr static auto LevelStr = std::to_array({
		"DBG", // Debug
		"INF", // Info
		"WRN", // Warn
		"ERR", // Error
	});

	constexpr static auto Colors = std::to_array({
		"\x1B[32m", // Debug
		"\x1B[36m", // Info
		"\x1B[33m", // Warn
		"\x1B[31m", // Error
		"\033[0m",  // Clear
	});

	friend Service<Logger_impl>;

	Logger_impl(Level minLevel, std::string_view filename):
		minLevel(minLevel)
	{
		auto* file = std::fopen(std::string(filename).c_str(), "w");
		if (!file) throw runtime_error_fmt(
			"Couldn't open log file \"{}\" for writing: {}",
			filename,
			std::strerror(errno)
		);
		logfile = FilePtr(file);
	}

	// Moveable
	Logger_impl(Logger_impl&&) = default;
	auto operator=(Logger_impl&&) -> Logger_impl& = default;

	using FilePtr = std::unique_ptr<std::FILE, decltype([](auto* f) {
		std::fclose(f);
	})>;

	Level minLevel;
	FilePtr logfile;
};

export using LogLevel = Logger_impl::Level;
export using Logger = Service<Logger_impl>;

// Mixin that makes logger service access more convenient
export class Log:
	Logger
{
public:
	template<typename... Args>
	void debug(std::string_view fmt, Args&&... args) {
		Logger::serv->log(LogLevel::Debug, fmt, std::forward<Args>(args)...);
	}

	template<typename... Args>
	void info(std::string_view fmt, Args&&... args) {
		Logger::serv->log(LogLevel::Info, fmt, std::forward<Args>(args)...);
	}

	template<typename... Args>
	void warn(std::string_view fmt, Args&&... args) {
		Logger::serv->log(LogLevel::Warn, fmt, std::forward<Args>(args)...);
	}

	template<typename... Args>
	void error(std::string_view fmt, Args&&... args) {
		Logger::serv->log(LogLevel::Error, fmt, std::forward<Args>(args)...);
	}
};
