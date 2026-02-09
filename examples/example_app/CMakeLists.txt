cmake_minimum_required(VERSION 3.30)

# Build the executable
add_executable(example_app main.cpp)

# Link to engine (which transitively links renderer and core)
target_link_libraries(example_app PRIVATE hammock_engine PRIVATE hammock_dependency_graph)

# Include directories for legacy headers
target_include_directories(example_app PUBLIC
    "${PROJECT_SOURCE_DIR}/third_party"
    "${PROJECT_SOURCE_DIR}/src/core"
    "${PROJECT_SOURCE_DIR}/src/renderer"
    "${PROJECT_SOURCE_DIR}/src/engine"
    "${PROJECT_SOURCE_DIR}/src/dependency_graph"
)


# Compiler features
target_compile_features(example_app PUBLIC cxx_std_23)

