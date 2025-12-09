project(hammock_renderer LANGUAGES CXX)

# Add core as a subdirectory (uses core/CMakeLists.txt)
add_subdirectory(
        "${CMAKE_CURRENT_LIST_DIR}/../core"
        core_build
)

file(GLOB RENDERER_SOURCES
        "*.cpp"
        "*.h"
)

add_library(hammock_renderer ${RENDERER_SOURCES})

target_include_directories(hammock_renderer
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(hammock_renderer PUBLIC "${CMAKE_CURRENT_LIST_DIR}/../external")

target_link_libraries(hammock_renderer
        PUBLIC
        hammock_core
)

target_compile_features(hammock_renderer PUBLIC cxx_std_23)
