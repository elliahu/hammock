cmake_minimum_required(VERSION 3.30)

# Renderer library
add_library(hammock_utils
        "filesystem.hpp"
        "math.hpp"

        "filesystem_impl.cpp"
        "math.cpp"
)


# Include directories for legacy headers
target_include_directories(hammock_utils PUBLIC
    "${PROJECT_SOURCE_DIR}/third_party"
    "${PROJECT_SOURCE_DIR}/src/core"
)

# Link core (which provides Vulkan transitively)
target_link_libraries(hammock_utils PUBLIC hammock_core hammock_dependency_graph)

# Compiler features
target_compile_features(hammock_utils PUBLIC cxx_std_23)
