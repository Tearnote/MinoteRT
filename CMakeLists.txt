### Project configuration
cmake_minimum_required(VERSION 3.23)
project(MinoteRT
        VERSION 0.0
        DESCRIPTION "A personal realtime ray-tracing renderer project"
        LANGUAGES C CXX)
set(CMAKE_C_STANDARD 11)          # For 3rd party C code
set(CMAKE_C_STANDARD_REQUIRED ON) # ^
set(CMAKE_C_EXTENSIONS OFF)       # ^
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

### Global build settings
set(BUILD_SHARED_LIBS OFF) # Only build static libs
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug,RelWithDebInfo>:Debug>") # Always use MT stdlib
add_compile_options("/arch:AVX2;/fp:fast") # Enable non standard speedups
add_compile_options("/EHsc") # Use standard C++ exceptions
add_compile_options("/wd5105") # Disable common false positives
add_compile_options("$<$<CONFIG:Debug>:/RTC1>") # Enable runtime safety checks
add_compile_options("$<$<CONFIG:Release>:/Gw;/Gy;/GL>") # Enable LTO
add_link_options("$<$<CONFIG:Release>:/OPT:ICF>")       # ^
# Communicate build type to the project
add_compile_definitions("BUILD_TYPE=$<$<CONFIG:Debug>:BUILD_DEBUG>$<$<CONFIG:RelWithDebInfo>:BUILD_RELDEB>$<$<CONFIG:Release>:BUILD_RELEASE>")

### Dependencies
include(cmake/Dependencies.cmake)

### Main executable
add_executable(MinoteRT
	src/sys/glfw.hpp src/sys/glfw.cpp
	src/util/service.hpp
	src/stx/concepts.hpp
	src/stx/except.hpp
	src/stx/ranges.hpp
	src/stx/verify.hpp
	src/stx/time.hpp
	src/config.hpp
	src/types.hpp
	src/math.hpp
	src/main.cpp
	src/log.hpp src/log.cpp
)
target_include_directories(MinoteRT PRIVATE src) # All includes start from src as root

target_link_options(MinoteRT PRIVATE "/SUBSYSTEM:WINDOWS")
target_link_libraries(MinoteRT PRIVATE winmm)

target_link_libraries(MinoteRT PRIVATE fmtlog)
target_link_libraries(MinoteRT PRIVATE assert)
target_link_libraries(MinoteRT PRIVATE volk)
target_link_libraries(MinoteRT PRIVATE glfw)
