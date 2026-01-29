cmake_minimum_required(VERSION 3.30)

# Renderer library
add_library(hammock_renderer
        "hammock_renderer.hpp"
        "spirv_reflection.hpp"
        "renderer.hpp"
        "base_rendering_strategy.hpp"
        "deferred_rendering_strategy.hpp"
        "filesystem.hpp"
        "scene.hpp"
        "math.hpp"
        "thread_pool.hpp"
        "vertex.hpp"
        "dependency_graph.hpp"
        "dependency_graph_compiler.hpp"
        "dependency_graph_executor.hpp"
        "gpu_task.hpp"
        "push_constants.hpp"
        "graphics_context.hpp"

        "filesystem_impl.cpp"
        "graphics_context.cpp"
        "renderer.cpp"
        "gpu_task.cpp"
        "math.cpp"
        "dependency_graph.cpp"
        "dependency_graph_compiler.cpp"
        "dependency_graph_executor.cpp"
        "deferred_rendering_strategy.cpp"
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

# Enable automatic module dependency scanning
set_target_properties(hammock_renderer PROPERTIES
        CXX_SCAN_FOR_MODULES ON
)