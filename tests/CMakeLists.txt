# 1. Create the test executable
add_executable(unit_tests test_dependency_graph.cpp)

# 2. Link GoogleTest AND your core_logic library
target_link_libraries(unit_tests PRIVATE 
    GTest::gtest_main 
    hammock_engine
    hammock_dependency_graph
)

# Include directories for legacy headers
target_include_directories(unit_tests PUBLIC
    "${PROJECT_SOURCE_DIR}/third_party"
    "${PROJECT_SOURCE_DIR}/src/core"
    "${PROJECT_SOURCE_DIR}/src/renderer"
    "${PROJECT_SOURCE_DIR}/src/engine"
    "${PROJECT_SOURCE_DIR}/src/dependency_graph"
    "${PROJECT_SOURCE_DIR}/third_party/imgui"
    "${Vulkan_INCLUDE_DIR}" # imgui needs vulkan.h
)



# 3. Register with CTest (for VS Code discovery)
gtest_discover_tests(unit_tests)