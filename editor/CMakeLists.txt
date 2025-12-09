project(hammock_editor LANGUAGES CXX)

# Add renderer as a subdirectory (uses renderer/CMakeLists.txt)
add_subdirectory(
        "${CMAKE_CURRENT_LIST_DIR}/../renderer"
        core_build
)

file(GLOB RENDERER_SOURCES "*.cpp")

add_library(hammock_editor ${RENDERER_SOURCES})

target_include_directories(hammock_editor
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(hammock_editor PUBLIC "${CMAKE_CURRENT_LIST_DIR}/../external")

target_link_libraries(hammock_editor
        PUBLIC
        hammock_renderer
)

target_compile_features(hammock_editor PUBLIC cxx_std_23)
