cmake_minimum_required(VERSION 3.30)

# Core library
add_library(hammock_core
        "hammock_core.hpp"
        "base_pipeline.hpp"
        "base_resource.hpp"
        "base_surface_provider.hpp"
        "buffer.hpp"
        "command_buffer.hpp"
        "compute_pipeline.hpp"
        "descriptors.hpp"
        "device.hpp"
        "swapchain_manager.hpp"
        "graphics_pipeline.hpp"
        "image.hpp" 
        "instance.hpp"
        "memory_allocator.hpp"
        "resource_manager.hpp"
        "swapchain.hpp"
        "utilities.hpp"
        "semaphore.hpp"

        "buffer.cpp"
        "image.cpp"
        "instance.cpp"
        "command_buffer.cpp"
        "compute_pipeline.cpp"
        "descriptors.cpp"
        "device.cpp"
        "swapchain_manager.cpp"
        "graphics_pipeline.cpp"
        "memory_allocator_impl.cpp"
        "resource_manager.cpp"
        "swapchain.cpp"
        "semaphore.cpp"
)

# Include directories for legacy headers
target_include_directories(hammock_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    "${PROJECT_SOURCE_DIR}/third_party"
    "${Vulkan_INCLUDE_DIR}"
    "${PROJECT_SOURCE_DIR}/src/core"
    "${PROJECT_SOURCE_DIR}/src/renderer"
    "${PROJECT_SOURCE_DIR}/src/engine"
)

target_link_libraries(hammock_core PUBLIC Vulkan::Vulkan)

# Compiler features
target_compile_features(hammock_core PUBLIC cxx_std_23)

# Enable automatic module dependency scanning
set_target_properties(hammock_core PROPERTIES
        CXX_SCAN_FOR_MODULES ON
)
