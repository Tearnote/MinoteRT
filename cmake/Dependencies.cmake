include_guard()

include(FetchContent)

### Local dependencies

find_package(Vulkan REQUIRED
	COMPONENTS glslc
)

### Remote dependencies

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(glfw
	GIT_REPOSITORY https://github.com/glfw/glfw
	GIT_TAG 3.3.8
)
FetchContent_MakeAvailable(glfw)

set(VOLK_PULL_IN_VULKAN OFF CACHE BOOL "" FORCE)
FetchContent_Declare(volk
	GIT_REPOSITORY https://github.com/zeux/volk
	GIT_TAG 98a66a5b95f00cc88dda15e1633708d2b316a3e4
)
FetchContent_MakeAvailable(volk)
target_compile_definitions(volk PUBLIC VK_NO_PROTOTYPES)
target_include_directories(volk PUBLIC ${Vulkan_INCLUDE_DIRS})
target_compile_definitions(volk PUBLIC VK_USE_PLATFORM_WIN32_KHR)

FetchContent_Declare(vk-bootstrap
	GIT_REPOSITORY https://github.com/charles-lunarg/vk-bootstrap
	GIT_TAG 61f77612c70dd49a59157fe139a7d248a90e206a
)
FetchContent_MakeAvailable(vk-bootstrap)

set(VUK_LINK_TO_LOADER OFF CACHE BOOL "" FORCE)
set(VUK_USE_SHADERC OFF CACHE BOOL "" FORCE)
set(VUK_FAIL_FAST ON CACHE BOOL "" FORCE)
FetchContent_Declare(vuk
	GIT_REPOSITORY https://github.com/martty/vuk
	GIT_TAG bb425aea68fa92c5ca94f712f97cadac119241bf
)
FetchContent_MakeAvailable(vuk)
target_compile_definitions(vuk PUBLIC VUK_CUSTOM_VULKAN_HEADER=<volk.h>)
target_link_libraries(vuk PRIVATE volk)

FetchContent_Declare(lodepng
	GIT_REPOSITORY https://github.com/lvandeve/lodepng
	GIT_TAG c18b949b71f45e78b1f9a28c5d458bce0da505d6
)
FetchContent_MakeAvailable(lodepng)
add_library(lodepng
	${lodepng_SOURCE_DIR}/lodepng.h ${lodepng_SOURCE_DIR}/lodepng.cpp
)
target_include_directories(lodepng PUBLIC ${lodepng_SOURCE_DIR})

FetchContent_Declare(imgui
	GIT_REPOSITORY https://github.com/ocornut/imgui
	GIT_TAG v1.89.6
)
FetchContent_MakeAvailable(imgui)
add_library(imgui
	${imgui_SOURCE_DIR}/imgui.h ${imgui_SOURCE_DIR}/imgui.cpp
	${imgui_SOURCE_DIR}/imconfig.h ${imgui_SOURCE_DIR}/imgui_internal.h
	${imgui_SOURCE_DIR}/imgui_draw.cpp ${imgui_SOURCE_DIR}/imgui_tables.cpp
	${imgui_SOURCE_DIR}/imgui_widgets.cpp ${imgui_SOURCE_DIR}/imgui_demo.cpp
	${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.h ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
)
target_compile_definitions(imgui PUBLIC IMGUI_DISABLE_OBSOLETE_FUNCTIONS)
target_compile_definitions(imgui PUBLIC IMGUI_DISABLE_OBSOLETE_KEYIO)
target_compile_definitions(imgui PUBLIC IMGUI_DISABLE_WIN32_FUNCTIONS)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR})
target_link_libraries(imgui PRIVATE glfw)

### Enable C++ modules support
FetchContent_Declare(cmake_for_modules
	GIT_REPOSITORY https://github.com/GabrielDosReis/cmake-for-modules
	GIT_TAG 51fc9d9a5d15002ab3bd98ca268143671e4d4772
)
FetchContent_MakeAvailable(cmake_for_modules)
include(${cmake_for_modules_SOURCE_DIR}/CXXModuleExperimentalSupport.cmake)
