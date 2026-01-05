cmake_minimum_required(VERSION 3.30)

# Core library
add_library(hammock_core)

# Add module sources
target_sources(hammock_core
    PUBLIC
        FILE_SET CXX_MODULES TYPE CXX_MODULES
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES
        "hammock_core.cppm"
        "base_pipeline.cppm"
        "base_resource.cppm"
        "base_surface_provider.cppm"
        "buffer.cppm"
        "command_buffer.cppm"
        "compute_pipeline.cppm"
        "descriptors.cppm"
        "device.cppm"
        "swapchain_manager.cppm"
        "graphics_pipeline.cppm"
        "image.cppm"
        "instance.cppm"
        "memory_allocator.cppm"
        "resource_manager.cppm"
        "swapchain.cppm"
        "utilities.cppm"
        "semaphore.cppm"
    PRIVATE
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
)

target_link_libraries(hammock_core PUBLIC Vulkan::Vulkan)

add_compile_definitions(VULKAN_HPP_NO_CONSTRUCTORS)
add_compile_definitions(VULKAN_HPP_NO_SPACESHIP_OPERATOR)

# Compiler features
target_compile_features(hammock_core PUBLIC cxx_std_23)

# Enable automatic module dependency scanning
set_target_properties(hammock_core PROPERTIES
        CXX_SCAN_FOR_MODULES ON
)
