cmake_minimum_required(VERSION 3.30)


# Editor library
add_library(hammock_engine)

# Add module sources
target_sources(hammock_engine
    PUBLIC
        FILE_SET CXX_MODULES TYPE CXX_MODULES
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
        FILES
        "Editor.cppm"
        "Engine.cppm"
        "Window.cppm"
        "Ui.cppm"
        "RgfwImguiImpl.cppm"
    PRIVATE
        "RgfwImguiImpl.cpp"
)

# Include directories for legacy headers
target_include_directories(hammock_engine PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        "${PROJECT_SOURCE_DIR}/third_party"
        "${PROJECT_SOURCE_DIR}/third_party/imgui"
)

# Link renderer (which links core transitively)
target_link_libraries(hammock_engine PUBLIC hammock_renderer)

# Compiler features
target_compile_features(hammock_engine PUBLIC cxx_std_23)

# Enable automatic module dependency scanning
set_target_properties(hammock_engine PROPERTIES
        CXX_SCAN_FOR_MODULES ON
)