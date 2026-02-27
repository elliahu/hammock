cmake_minimum_required(VERSION 3.30)

# Renderer library
add_library(hammock_ui
        "ui.hpp"
        "font.hpp"

        "ui.cpp"
        "font.cpp"
)


# Include directories for legacy headers
target_include_directories(hammock_ui PUBLIC
    "${PROJECT_SOURCE_DIR}/third_party"
    "${PROJECT_SOURCE_DIR}/src"
)

# Link core (which provides Vulkan transitively)
target_link_libraries(hammock_ui PUBLIC hammock_core hammock_renderer hammock_utils)

# Compiler features
target_compile_features(hammock_ui PUBLIC cxx_std_23)
