cmake_minimum_required(VERSION 3.30)

set(CMAKE_CXX_SCAN_FOR_MODULES ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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
# This target compiles slang shaders into binary output directory
add_subdirectory(src/renderer/shaders)
add_subdirectory(src/engine)

# Add examples
add_subdirectory(examples)