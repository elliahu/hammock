# Add the executable
add_executable(sponza_scene
        main.cpp
)

# Link the engine library
target_link_libraries(sponza_scene PRIVATE hammock)
target_include_directories(sponza_scene PRIVATE ${PROJECT_SOURCE_DIR}/include)