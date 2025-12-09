cmake_minimum_required(VERSION 3.29)
project(hammock_core LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find all .cpp files in this directory
file(GLOB CORE_SOURCES
        "*.cpp"
        "*.h"
)

add_library(hammock_core ${CORE_SOURCES})

add_compile_definitions(NOMINMAX)
add_compile_definitions(WIN32_LEAN_AND_MEAN)

# Make this directory available for includes
target_include_directories(hammock_core
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(hammock_core PUBLIC "${CMAKE_CURRENT_LIST_DIR}/../external")

if(WIN32)
    target_compile_definitions(hammock_core PRIVATE VK_USE_PLATFORM_WIN32_KHR)

elseif(UNIX AND NOT APPLE)
    target_compile_definitions(hammock_core PRIVATE VK_USE_PLATFORM_XLIB_KHR)
elseif(APPLE)
    target_compile_definitions(hammock_core PRIVATE VK_USE_PLATFORM_METAL_EXT)
endif()

# Link vulkan
find_package(Vulkan REQUIRED )
target_link_libraries(hammock_core PUBLIC Vulkan::Vulkan)

target_compile_features(hammock_core PUBLIC cxx_std_23)