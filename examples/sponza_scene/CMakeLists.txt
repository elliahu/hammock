cmake_minimum_required(VERSION 3.29)
project(sponza_scene LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Bring in editor -> renderer → automatically brings in core
add_subdirectory(
        "${CMAKE_CURRENT_LIST_DIR}/../../src/editor"
        renderer_build
)

# Build the executable
add_executable(sponza_scene
        main.cpp
)

# Link to the renderer (which links core)
target_link_libraries(sponza_scene
        PRIVATE
        hammock_renderer
)

target_compile_features(sponza_scene PUBLIC cxx_std_20)
