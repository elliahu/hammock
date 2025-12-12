project(hammock_renderer LANGUAGES CXX)

#
# Add core (which may contain modules)
#
add_subdirectory(
        "${CMAKE_CURRENT_LIST_DIR}/../core"
        core_build
)

#
# Collect module interfaces, implementation files
#
file(GLOB RENDERER_MODULE_IFACES "${CMAKE_CURRENT_SOURCE_DIR}/*.cppm")
file(GLOB RENDERER_SOURCES      "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

#
# Renderer library with module interface file set
#
add_library(hammock_renderer)

# Register module interfaces
target_sources(hammock_renderer
        PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES
        ${RENDERER_MODULE_IFACES}
)

# Add implementation units
target_sources(hammock_renderer
        PRIVATE
        ${RENDERER_SOURCES}
)

#
# Module search roots (required for module import resolution)
#
target_sources(hammock_renderer
        PUBLIC
        FILE_SET cxx_modules BASE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}
)

#
# External includes (only needed if legacy headers are included)
#
target_include_directories(hammock_renderer PUBLIC
        "${CMAKE_CURRENT_LIST_DIR}/../../external"
)

#
# Link core
#
target_link_libraries(hammock_renderer
        PUBLIC
        hammock_core
)

#
# C++ features
#
target_compile_features(hammock_renderer PUBLIC cxx_std_23)
