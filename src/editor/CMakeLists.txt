project(hammock_editor LANGUAGES CXX)

#
# Add the renderer (which also uses modules)
#
add_subdirectory(
        "${CMAKE_CURRENT_LIST_DIR}/../renderer"
        core_build
)

#
# Collect sources
#
# Module interfaces (.cppm)
file(GLOB EDITOR_MODULE_IFACES "${CMAKE_CURRENT_SOURCE_DIR}/*.cppm")

# Module implementation units or normal sources (.cpp)
file(GLOB EDITOR_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

#
# Library definition
#
add_library(hammock_editor)

# Add module interfaces as module file set
target_sources(hammock_editor
        PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES
        ${EDITOR_MODULE_IFACES}
)

# Add implementation sources
target_sources(hammock_editor
        PRIVATE
        ${EDITOR_SOURCES}
)

#
# Module search path
# (Modules do NOT use include directories for module imports)
#
target_sources(hammock_editor
        PUBLIC
        FILE_SET cxx_modules BASE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}
)

#
# Link dependencies
#
target_link_libraries(hammock_editor
        PUBLIC
        hammock_renderer
)

#
# Compiler features
#
target_compile_features(hammock_editor
        PUBLIC cxx_std_23
)
