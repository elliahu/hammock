cmake_minimum_required(VERSION 3.30)


# Editor library
add_library(hammock_editor)

# Add module sources
target_sources(hammock_editor
    PUBLIC
        FILE_SET CXX_MODULES TYPE CXX_MODULES
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES
        "Editor.cppm"
    PRIVATE

)

# Link renderer (which links core transitively)
target_link_libraries(hammock_editor PUBLIC hammock_renderer)

# Compiler features
target_compile_features(hammock_editor PUBLIC cxx_std_23)

# Enable automatic module dependency scanning
set_target_properties(hammock_editor PROPERTIES
        CXX_SCAN_FOR_MODULES ON
)