cmake_minimum_required(VERSION 3.30)

# Build the executable
add_executable(example_app main.cpp)

# Link to engine (which transitively links renderer and core)
target_link_libraries(example_app PRIVATE hammock_engine)

# Compiler features
target_compile_features(example_app PUBLIC cxx_std_23)