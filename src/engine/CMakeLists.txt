cmake_minimum_required(VERSION 3.30)

# Imgui is compiled with this target
set(IMGUI_DIR "${PROJECT_SOURCE_DIR}/third_party/imgui")

# Editor library
add_library(hammock_engine
        "hammock_engine.hpp"
        "runner.hpp"
        "application.hpp"
        "window.hpp"
        "ui.hpp"
        "framebuffer.hpp"
        "presentation_engine.hpp"

        "runner.cpp"
        "application.cpp"
        "framebuffer.cpp"
        "ui.cpp"
        "window.cpp"
        "presentation_engine.cpp"
        "${IMGUI_DIR}/imgui.cpp"
        "${IMGUI_DIR}/imgui_draw.cpp"
        "${IMGUI_DIR}/imgui_tables.cpp"
        "${IMGUI_DIR}/imgui_widgets.cpp"
        "${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp"
)




# Include directories for legacy headers
target_include_directories(hammock_engine PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        "${PROJECT_SOURCE_DIR}/third_party"
        "${PROJECT_SOURCE_DIR}/third_party/imgui"
        "${Vulkan_INCLUDE_DIR}" # imgui needs vulkan.h
        "${PROJECT_SOURCE_DIR}/src/core"
        "${PROJECT_SOURCE_DIR}/src/renderer"
        "${PROJECT_SOURCE_DIR}/src/engine"
)

# Link renderer (which links core transitively)
target_link_libraries(hammock_engine PUBLIC hammock_renderer)

# Platform-specific libraries required by VulkanSurfer
# Here I added them manually, another option is to add Surfers Cmake as subdirectory here
if(WIN32)
    target_link_libraries(hammock_engine INTERFACE Dwmapi)
elseif(UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
    target_link_libraries(hammock_engine INTERFACE ${X11_LIBRARIES})
endif()

# Compiler features
target_compile_features(hammock_engine PUBLIC cxx_std_23)

# Enable automatic module dependency scanning
set_target_properties(hammock_engine PROPERTIES
        CXX_SCAN_FOR_MODULES ON
)