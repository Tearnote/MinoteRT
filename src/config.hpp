#pragma once

import <array>;
import minote.log;

inline constexpr auto AppTitle = "Minote";
inline constexpr auto AppVersion = std::to_array({ 0u, 0u, 0u });

// Make sure BUILD_TYPE is specified
#define BUILD_DEBUG 0
#define BUILD_RELDEB 1
#define BUILD_RELEASE 2
#ifndef BUILD_TYPE
#error Build type not defined
#endif
#if (BUILD_TYPE != BUILD_DEBUG) && (BUILD_TYPE != BUILD_RELDEB) && (BUILD_TYPE != BUILD_RELEASE)
#error Build type incorrectly defined
#endif

// Whether to disable assertions
#ifdef NDEBUG
#undef NDEBUG
#endif
#if BUILD_TYPE == BUILD_RELEASE
#define NDEBUG
#endif

// Whether to name threads for debugging
#if BUILD_TYPE != BUILD_RELEASE
#define THREAD_DEBUG
#endif

// Level of logging to file and/or console
#if BUILD_TYPE != BUILD_RELEASE
inline constexpr auto LoggingLevel = LogLevel::Debug;
#else
inline constexpr auto LoggingLevel = LogLevel::Info;
#endif

// Whether Vulkan validation layers are enabled
#if BUILD_TYPE == BUILD_DEBUG
#define VK_VALIDATION
#endif

// Logfile location
#if BUILD_TYPE == BUILD_DEBUG
inline constexpr auto const* LogfilePath = "minote-debug.log";
#elif BUILD_TYPE == BUILD_RELDEB
inline constexpr auto const* LogfilePath = "minote-reldeb.log";
#else
inline constexpr auto const* LogfilePath = "minote.log";
#endif
