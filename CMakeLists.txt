cmake_minimum_required(VERSION 3.30)

set(CMAKE_CXX_SCAN_FOR_MODULES ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(hammock LANGUAGES CXX)


# Platform definitions
if(WIN32)
    add_compile_definitions(
        NOMINMAX
        WIN32_LEAN_AND_MEAN
        VK_USE_PLATFORM_WIN32_KHR
    )
elseif(UNIX AND NOT APPLE)
    add_compile_definitions(VK_USE_PLATFORM_XLIB_KHR)
elseif(APPLE)
    add_compile_definitions(VK_USE_PLATFORM_METAL_EXT)
endif()

add_compile_definitions(
    VULKAN_HPP_NO_SPACESHIP_OPERATOR
    VULKAN_HPP_NO_CONSTRUCTORS
)

# Find Vulkan once for the whole project
find_package(Vulkan REQUIRED)

# Require Vulkan version ≥ 1.3.256 (earliest version when the Vulkan module was available)
if( ${Vulkan_VERSION} VERSION_LESS "1.3.296" )
    message( FATAL_ERROR "Minimum required Vulkan version 1.3.256. "
            "Found ${Vulkan_VERSION}."
    )
endif()

# Add libraries in dependency order
add_subdirectory(src/core)
add_subdirectory(src/renderer)
add_subdirectory(src/dependency_graph)
# This target compiles slang shaders into binary output directory
add_subdirectory(src/renderer/shaders)
add_subdirectory(src/app)
add_subdirectory(src/utils)


# Add examples
add_subdirectory(examples)


# Google test
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
enable_testing()
include(GoogleTest)
add_subdirectory(tests)