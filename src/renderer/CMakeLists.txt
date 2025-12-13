cmake_minimum_required(VERSION 3.30)

# Collect module interfaces and implementation files
file(GLOB_RECURSE RENDERER_MODULE_IFACES "${CMAKE_CURRENT_SOURCE_DIR}/*.cppm")
file(GLOB_RECURSE RENDERER_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

# Renderer library
add_library(hammock_renderer)

# Add module sources
target_sources(hammock_renderer
    PUBLIC
        FILE_SET CXX_MODULES TYPE CXX_MODULES
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES ${RENDERER_MODULE_IFACES}
    PRIVATE
        ${RENDERER_SOURCES}
)

# Include directories for legacy headers
target_include_directories(hammock_renderer PUBLIC
    "${PROJECT_SOURCE_DIR}/external"
)

# Link core (which provides Vulkan transitively)
target_link_libraries(hammock_renderer PUBLIC hammock_core)

# Compiler features
target_compile_features(hammock_renderer PUBLIC cxx_std_23)

# Enable automatic module dependency scanning
set_target_properties(hammock_renderer PROPERTIES
        CXX_SCAN_FOR_MODULES ON
)