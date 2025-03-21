# Add the executable
add_executable(cloud_renderer
        main.cpp
        # scenes
        scenes/IScene.h
        scenes/SkyScene.cpp
        scenes/SkyScene.h
        scenes/NoiseEditor.cpp
        scenes/NoiseEditor.h
        scenes/ParticipatingMediumScene.cpp
        scenes/ParticipatingMediumScene.h
        scenes/SignedDistanceField.h

        # renderer
        renderer/Renderer.h
        renderer/Renderer.cpp
        renderer/Types.h
        renderer/Camera.h
        renderer/UserInterface.h
        renderer/UserInterface.cpp

)

# Link the engine library
target_link_libraries(cloud_renderer PRIVATE hammock)
target_include_directories(cloud_renderer PRIVATE ${PROJECT_SOURCE_DIR}/include)