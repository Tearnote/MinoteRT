module;

#ifndef NOMINMAX
#define NOMINMAX
#endif //NOMINMAX
#ifdef WIN32_LEAN_AND_MEAN
#undef WIN32_LEAN_AND_MEAN
#endif //WIN32_LEAN_AND_MEAN
#include <Windows.h>

export module minote.os;

import <string_view>;
import <stdexcept>;
import <utility>;
import <fcntl.h>;
import <string>;
import <cstdio>;
import <io.h>;
import "config.hpp";
import minote.time;

export class SchedulerPeriod {
public:
	explicit SchedulerPeriod(nsec period):
		period(period)
	{
		if (timeBeginPeriod(period) != TIMERR_NOERROR)
			throw std::runtime_error("Failed to initialize Windows timer");
	}

	~SchedulerPeriod() {
		if (period == -1ll) return;
		timeEndPeriod(1);
	}

	SchedulerPeriod(SchedulerPeriod&& other) {
		*this = std::move(other);
	}

	auto operator=(SchedulerPeriod&& other) -> SchedulerPeriod& {
		period = other.period;
		other.period = -1ll;
	}

private:
	nsec period;
};

export void setThreadName(std::string_view name) {
#ifdef THREAD_DEBUG
	auto lname = std::wstring(name.begin(), name.end());
	SetThreadDescription(GetCurrentThread(), lname.c_str());
#endif //THREAD_DEBUG
}

// https://github.com/ocaml/ocaml/issues/9252#issuecomment-576383814
export void createConsole() {
	AllocConsole();

	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);

	int fdOut = _open_osfhandle(reinterpret_cast<intptr_t>(GetStdHandle(STD_OUTPUT_HANDLE)), _O_WRONLY | _O_BINARY);
	int fdErr = _open_osfhandle(reinterpret_cast<intptr_t>(GetStdHandle(STD_ERROR_HANDLE)), _O_WRONLY | _O_BINARY);

	if (fdOut) {
		_dup2(fdOut, 1);
		_close(fdOut);
		SetStdHandle(STD_OUTPUT_HANDLE, reinterpret_cast<HANDLE>(_get_osfhandle(1)));
	}
	if (fdErr) {
		_dup2(fdErr, 2);
		_close(fdErr);
		SetStdHandle(STD_ERROR_HANDLE, reinterpret_cast<HANDLE>(_get_osfhandle(2)));
	}

	_dup2(_fileno(fdopen(1, "wb")), _fileno(stdout));
	_dup2(_fileno(fdopen(2, "wb")), _fileno(stderr));

	std::setvbuf(stdout, nullptr, _IONBF, 0);
	std::setvbuf(stderr, nullptr, _IONBF, 0);

	// Set console encoding to UTF-8
	SetConsoleOutputCP(65001);

	// Enable ANSI color code support
	auto out = GetStdHandle(STD_OUTPUT_HANDLE);
	auto mode = 0ul;
	GetConsoleMode(out, &mode);
	mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	SetConsoleMode(out, mode);
}
