cmake_minimum_required(VERSION 3.30)

# Renderer library
add_library(hammock_dependency_graph
    "hammock_dependency_graph.hpp"
    "dependency_graph_compiler.cpp"
    "dependency_graph_compiler.hpp"
    "dependency_graph_executor.cpp"
    "dependency_graph_executor.hpp"
    "dependency_graph.cpp"
    "dependency_graph.hpp"
    "gpu_task.cpp"
    "gpu_task.hpp"
    "push_constants.hpp"
)


# Include directories for legacy headers
target_include_directories(hammock_dependency_graph PUBLIC
    "${PROJECT_SOURCE_DIR}/third_party"
    "${PROJECT_SOURCE_DIR}/src/core"
    "${PROJECT_SOURCE_DIR}/src/renderer"
    "${PROJECT_SOURCE_DIR}/src/dependency_graph"
    "${PROJECT_SOURCE_DIR}/src/engine"
)

# Link core (which provides Vulkan transitively)
target_link_libraries(hammock_dependency_graph PUBLIC hammock_core)

# Compiler features
target_compile_features(hammock_dependency_graph PUBLIC cxx_std_23)
