cmake_minimum_required(VERSION 3.30)

# Core library
add_library(hammock_core)

# Add module sources
target_sources(hammock_core
    PUBLIC
        FILE_SET CXX_MODULES TYPE CXX_MODULES
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES
        "import.cppm"
        "BasePipeline.cppm"
        "BaseResource.cppm"
        "BaseSurfaceProvider.cppm"
        "Buffer.cppm"
        "CommandBuffer.cppm"
        "ComputePipeline.cppm"
        "Descriptor.cppm"
        "Device.cppm"
        "SwapChainManager.cppm"
        "GraphicsPipeline.cppm"
        "Image.cppm"
        "Instance.cppm"
        "MemoryAllocator.cppm"
        "ResourceManager.cppm"
        "SwapChain.cppm"
        "Utilities.cppm"
        "Semaphore.cppm"
    PRIVATE
        "CommandBuffer.cpp"
        "ComputePipeline.cpp"
        "Descriptor.cpp"
        "Device.cpp"
        "SwapChainManager.cpp"
        "GraphicsPipeline.cpp"
        "MemoryAllocator.cpp"
        "ResourceManager.cpp"
        "SwapChain.cpp"
        "Semaphore.cpp"
)

# Include directories for legacy headers
target_include_directories(hammock_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    "${PROJECT_SOURCE_DIR}/third_party"
    "${Vulkan_INCLUDE_DIR}"
)

target_link_libraries(hammock_core PUBLIC Vulkan::Vulkan)

add_compile_definitions(VULKAN_HPP_NO_CONSTRUCTORS)

# Compiler features
target_compile_features(hammock_core PUBLIC cxx_std_23)

# Enable automatic module dependency scanning
set_target_properties(hammock_core PROPERTIES
        CXX_SCAN_FOR_MODULES ON
)
