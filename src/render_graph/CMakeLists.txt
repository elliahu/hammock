cmake_minimum_required(VERSION 3.30)

# Renderer library
add_library(hammock_render_graph
    "render_graph_compiler.cpp"
    "render_graph_compiler.hpp"
    "render_graph_executor.cpp"
    "render_graph_executor.hpp"
    "render_graph.cpp"
    "render_graph.hpp"
    "render_pass.cpp"
    "render_pass.hpp"
)


# Include directories for legacy headers
target_include_directories(hammock_render_graph PUBLIC
    "${PROJECT_SOURCE_DIR}/third_party"
    "${PROJECT_SOURCE_DIR}/src"
)

# Link core (which provides Vulkan transitively)
target_link_libraries(hammock_render_graph PUBLIC hammock_core)

# Compiler features
target_compile_features(hammock_render_graph PUBLIC cxx_std_23)
