# src/CMakeLists.txt
add_subdirectory(hammock/core)
add_subdirectory(hammock/platform)
add_subdirectory(hammock/resources)
add_subdirectory(hammock/renderer)
add_subdirectory(hammock/rendergraph)
add_subdirectory(hammock/scene)
add_subdirectory(hammock/utils)
add_subdirectory(hammock/legacy)

# Combine all sources for parent scope
set(HAMMOCK_SRC
        ${CORE_SOURCES}
        ${PLATFORM_SOURCES}
        ${RESOURCE_SOURCES}
        ${RENDERGRAPH_SOURCES}
        ${SCENE_SOURCES}
        ${UTILS_SOURCES}
        ${LEGACY_SOURCES}
        PARENT_SCOPE
)