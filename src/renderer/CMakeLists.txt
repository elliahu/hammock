cmake_minimum_required(VERSION 3.30)

# Renderer library
add_library(hammock_renderer
        "hammock_renderer.hpp"
        "spirv_reflection.hpp"
        "renderer.hpp"
        "filesystem.hpp"
        "scene.hpp"
        "math.hpp"
        "thread_pool.hpp"
        "vertex.hpp"
        "graphics_context.hpp"

        "filesystem_impl.cpp"
        "graphics_context.cpp"
        "renderer.cpp"
        "math.cpp"
)


# Include directories for legacy headers
target_include_directories(hammock_renderer PUBLIC
    "${PROJECT_SOURCE_DIR}/third_party"
    "${PROJECT_SOURCE_DIR}/src/core"
    "${PROJECT_SOURCE_DIR}/src/renderer"
    "${PROJECT_SOURCE_DIR}/src/engine"
)

# Link core (which provides Vulkan transitively)
target_link_libraries(hammock_renderer PUBLIC hammock_core)

# Compiler features
target_compile_features(hammock_renderer PUBLIC cxx_std_23)
