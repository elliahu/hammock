cmake_minimum_required(VERSION 3.30)

# Build the executable
add_executable(sponza_scene main.cpp)

# Link to editor (which transitively links renderer and core)
target_link_libraries(sponza_scene PRIVATE hammock_editor)

# Compiler features
target_compile_features(sponza_scene PUBLIC cxx_std_23)