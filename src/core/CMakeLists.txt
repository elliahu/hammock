cmake_minimum_required(VERSION 3.29)
project(hammock_core LANGUAGES CXX)

#
# Collect module interfaces and implementation files
#
file(GLOB CORE_MODULE_IFACES "${CMAKE_CURRENT_SOURCE_DIR}/*.cppm")
file(GLOB CORE_SOURCES       "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

# Optional legacy headers (only if still needed)
file(GLOB CORE_HEADERS       "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

#
# Core library
#
add_library(hammock_core)

#
# Register module interfaces
#
target_sources(hammock_core
        PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES
        ${CORE_MODULE_IFACES}
)

#
# Register implementation units and optional headers
#
target_sources(hammock_core
        PRIVATE
        ${CORE_SOURCES}
        ${CORE_HEADERS}
)

#
# Module search paths
#
target_sources(hammock_core
        PUBLIC
        FILE_SET cxx_modules BASE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}
)

#
# Platform-specific definitions
#
target_compile_definitions(hammock_core PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
)

if (WIN32)
    target_compile_definitions(hammock_core PRIVATE VK_USE_PLATFORM_WIN32_KHR)
elseif (UNIX AND NOT APPLE)
    target_compile_definitions(hammock_core PRIVATE VK_USE_PLATFORM_XLIB_KHR)
elseif (APPLE)
    target_compile_definitions(hammock_core PRIVATE VK_USE_PLATFORM_METAL_EXT)
endif()

#
# External includes (only needed if legacy headers are included)
#
target_include_directories(hammock_core PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        "${CMAKE_CURRENT_LIST_DIR}/../../external"
)

#
# Vulkan
#
find_package(Vulkan REQUIRED)
target_link_libraries(hammock_core PUBLIC Vulkan::Vulkan)

#
# C++ features
#
target_compile_features(hammock_core PUBLIC cxx_std_23)
