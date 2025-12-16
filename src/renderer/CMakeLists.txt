cmake_minimum_required(VERSION 3.30)

# Renderer library
add_library(hammock_renderer)

# Add module sources
target_sources(hammock_renderer
    PUBLIC
        FILE_SET CXX_MODULES TYPE CXX_MODULES
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES
        "SpirvReflection.cppm"
        "Renderer.cppm"
        "Renderer_Strategy.cppm"
        "Renderer_DeferredStrategy.cppm"
        "Filesystem.cppm"
        "Geometry.cppm"
        "Math.cppm"
        "ThreadPool.cppm"
        "Vertex.cppm"
        "TaskGraph.cppm"
        "TaskGraph_Task.cppm"
        "TaskGraph_Socket.cppm"
        "TaskGraph_PushConstants.cppm"
        "TaskGraph_TaskFactory.cppm"
    PRIVATE
        "${PROJECT_SOURCE_DIR}/third_party/spirv_reflect/spirv_reflect.cpp" # include spirv_reflect source
        "Filesystem.cpp"
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