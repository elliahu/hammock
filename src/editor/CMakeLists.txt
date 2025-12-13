cmake_minimum_required(VERSION 3.30)

# Collect module interfaces and implementation files
file(GLOB_RECURSE EDITOR_MODULE_IFACES "${CMAKE_CURRENT_SOURCE_DIR}/*.cppm")
file(GLOB_RECURSE EDITOR_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

# Editor library
add_library(hammock_editor)

# Add module sources
target_sources(hammock_editor
    PUBLIC
        FILE_SET CXX_MODULES TYPE CXX_MODULES
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES ${EDITOR_MODULE_IFACES}
    PRIVATE
        ${EDITOR_SOURCES}
)

# Link renderer (which links core transitively)
target_link_libraries(hammock_editor PUBLIC hammock_renderer)

# Compiler features
target_compile_features(hammock_editor PUBLIC cxx_std_23)