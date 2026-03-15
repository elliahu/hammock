cmake_minimum_required(VERSION 3.30)

# Renderer library
add_library(hammock_renderer
        "spirv_reflection.hpp"
        "renderer.hpp"
        "render_frontend.hpp"
        "render_backend.hpp"
        "render_proxy.hpp"
        "vertex.hpp"
        "gltf_loader.hpp"
        "stage_types.hpp"
        "stage.hpp"
        "framebuffer.hpp"
        "presenter.hpp"


        "presenter.cpp"
        "renderer.cpp"
        "framebuffer.cpp"
        "stage.cpp"
        "gltf_loader.cpp"
        "cgltf_impl.cpp"
        "render_frontend.cpp"
        "render_backend.cpp"
        "render_proxy.cpp"
)

# Compile slang shaders
add_subdirectory(shaders)
add_dependencies(hammock_renderer process_slang_files)


# Include directories for legacy headers
target_include_directories(hammock_renderer PUBLIC
    "${PROJECT_SOURCE_DIR}/third_party"
    "${PROJECT_SOURCE_DIR}/src"
)

# Link core (which provides Vulkan transitively)
target_link_libraries(hammock_renderer PUBLIC hammock_core hammock_utils hammock_ui hammock_dependency_graph)

# Compiler features
target_compile_features(hammock_renderer PUBLIC cxx_std_23)
