cmake_minimum_required(VERSION 3.30)

# Renderer library
add_library(hammock_renderer)

# Add module sources
target_sources(hammock_renderer
    PUBLIC
        FILE_SET CXX_MODULES TYPE CXX_MODULES
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES
        "hammock_renderer.cppm"
        "spirv_reflection.cppm"
        "renderer.cppm"
        "base_rendering_strategy.cppm"
        "deferred_rendering_strategy.cppm"
        "filesystem.cppm"
        "scene.cppm"
        "math.cppm"
        "thread_pool.cppm"
        "vertex.cppm"
        "task_graph.cppm"
        "gpu_task.cppm"
        "socket.cppm"
        "push_constants.cppm"
        "graphics_context.cppm"
    PRIVATE
        "${PROJECT_SOURCE_DIR}/third_party/spirv_reflect/spirv_reflect.cpp" # include spirv_reflect source
        "filesystem_impl.cpp"
        "graphics_context.cpp"
        "renderer.cpp"
        "gpu_task.cpp"
        "socket.cpp"
        "task_graph.cpp"
        "deferred_rendering_strategy.cpp"
)

# Include directories for legacy headers
target_include_directories(hammock_renderer PUBLIC
    "${PROJECT_SOURCE_DIR}/third_party"
)

# Link core (which provides Vulkan transitively)
target_link_libraries(hammock_renderer PUBLIC hammock_core)

# Compiler features
target_compile_features(hammock_renderer PUBLIC cxx_std_23)

# Enable automatic module dependency scanning
set_target_properties(hammock_renderer PROPERTIES
        CXX_SCAN_FOR_MODULES ON
)