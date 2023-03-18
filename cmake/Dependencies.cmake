include_guard()

### System dependencies
include(FetchContent)
find_package(Vulkan REQUIRED)

### Fetched libraries
set(VOLK_PULL_IN_VULKAN OFF CACHE BOOL "" FORCE)
FetchContent_Declare(volk
	GIT_REPOSITORY https://github.com/zeux/volk
	GIT_TAG fdfa4937d416e01455325059930d15b5d2b1ba1f)
FetchContent_MakeAvailable(volk)
target_compile_definitions(volk PUBLIC VK_NO_PROTOTYPES)
target_include_directories(volk PUBLIC ${Vulkan_INCLUDE_DIRS})
target_compile_definitions(volk PUBLIC VK_USE_PLATFORM_WIN32_KHR)

set(VUK_LINK_TO_LOADER OFF CACHE BOOL "" FORCE)
set(VUK_USE_SHADERC OFF CACHE BOOL "" FORCE)
FetchContent_Declare(vuk
	GIT_REPOSITORY https://github.com/martty/vuk
	GIT_TAG a94873fd50a2f234dd084919c3ceb2d50a415c4b)
FetchContent_MakeAvailable(vuk)
target_compile_definitions(vuk PUBLIC VUK_CUSTOM_VULKAN_HEADER=<volk.h>)
target_link_libraries(vuk PRIVATE volk)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(glfw
	GIT_REPOSITORY https://github.com/glfw/glfw
	GIT_TAG 3.3.8)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(assert
	GIT_REPOSITORY https://github.com/jeremy-rifkin/libassert
	GIT_TAG a94970435d077666d8c712ececb5993c546732c2)
FetchContent_MakeAvailable(assert)

if(NOT TARGET fmt::fmt) # vuk already includes this
	FetchContent_Declare(fmt
		GIT_REPOSITORY https://github.com/fmtlib/fmt
		GIT_TAG 9.1.0)
	FetchContent_MakeAvailable(fmt)
endif()

FetchContent_Declare(fmtlog
	GIT_REPOSITORY https://github.com/MengRao/fmtlog
	GIT_TAG v2.2.1)
FetchContent_Populate(fmtlog) # The CMakeLists of fmtlog is only configured for install, not include
add_library(fmtlog INTERFACE ${fmtlog_SOURCE_DIR}/fmtlog.h ${fmtlog_SOURCE_DIR}/fmtlog-inl.h)
target_include_directories(fmtlog INTERFACE ${fmtlog_SOURCE_DIR})
target_compile_definitions(fmtlog INTERFACE FMTLOG_HEADER_ONLY)
target_compile_definitions(fmtlog INTERFACE FMT_NOEXCEPT=noexcept)
target_link_libraries(fmtlog INTERFACE fmt::fmt)

FetchContent_Declare(vk-bootstrap
	GIT_REPOSITORY https://github.com/charles-lunarg/vk-bootstrap
	GIT_TAG 61f77612c70dd49a59157fe139a7d248a90e206a)
FetchContent_MakeAvailable(vk-bootstrap)
