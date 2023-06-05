### Project configuration
cmake_minimum_required(VERSION 3.25)
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
add_compile_options("$<$<CONFIG:Debug>:/RTC1>") # Enable runtime safety checks
add_compile_options("$<$<CONFIG:Release>:/Gw;/Gy;/GL>") # Enable LTO
add_link_options("$<$<CONFIG:Release>:/OPT:ICF;/LTCG>") # ^
# Communicate build type to the project
add_compile_definitions("BUILD_TYPE=$<$<CONFIG:Debug>:BUILD_DEBUG>$<$<CONFIG:RelWithDebInfo>:BUILD_RELDEB>$<$<CONFIG:Release>:BUILD_RELEASE>")

include(cmake/Dependencies.cmake)

add_executable(MinoteRT WIN32
	src/stx/concepts.ixx
	src/stx/ranges.ixx
	src/stx/except.ixx
	src/stx/types.ixx
	src/stx/time.ixx
	src/stx/math.ixx
	src/util/service.ixx
	src/util/log.ixx
	src/sys/vulkan.ixx
	src/sys/window.ixx
	src/sys/os.ixx
	src/gfx/modules/pathtracer.ixx
	src/gfx/modules/tonemapper.ixx
	src/gfx/modules/denoiser.ixx
	src/gfx/modules/sky.ixx
	src/gfx/renderer.ixx
	src/gfx/samplers.ixx
	src/gfx/resource.ixx
	src/gfx/camera.ixx
	src/gfx/imgui.ixx
	src/freecam.ixx
	src/config.hpp
	src/main.cpp
	src/app.ixx
)
target_include_directories(MinoteRT PRIVATE src) # All includes start from src as root

target_link_libraries(MinoteRT PRIVATE winmm)

target_link_libraries(MinoteRT PRIVATE vk-bootstrap)
target_link_libraries(MinoteRT PRIVATE lodepng)
target_link_libraries(MinoteRT PRIVATE imgui)
target_link_libraries(MinoteRT PRIVATE glfw)
target_link_libraries(MinoteRT PRIVATE volk)
target_link_libraries(MinoteRT PRIVATE vuk)

### Assets and other artifacts
include(cmake/Shaders.cmake)
add_dependencies(MinoteRT MinoteRT_Shaders)
target_include_directories(MinoteRT PUBLIC ${PROJECT_BINARY_DIR}/$<CONFIG>/generated)
include(cmake/Assets.cmake)
add_dependencies(MinoteRT MinoteRT_Assets)
