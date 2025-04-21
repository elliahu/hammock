# Add the executable
add_executable(cloud_renderer
        main.cpp
        # scenes
        scenes/IScene.h
        scenes/ParticipatingMediumScene.cpp
        scenes/ParticipatingMediumScene.h
        scenes/SignedDistanceField.h

        # renderer
        renderer/Renderer.h
        renderer/Renderer.cpp
        renderer/Types.h
        renderer/Camera.h
        renderer/ui/UserInterface.h
        renderer/ui/UserInterface.cpp
        renderer/IRenderGroup.h
        renderer/geometry/GeometryPass.h
        renderer/geometry/GeometryPass.cpp
        renderer/clouds/CloudsPass.cpp
        renderer/clouds/CloudsPass.h
        renderer/composition/CompositionPass.cpp
        renderer/composition/CompositionPass.cpp
        renderer/composition/CompositionPass.h
        renderer/composition/PostProcessingPass.cpp
        renderer/composition/PostProcessingPass.h
        renderer/atmosphere/AtmospherePass.cpp
        renderer/atmosphere/AtmospherePass.h
        renderer/atmosphere/ILookUpTable.h
        renderer/atmosphere/Transmittance.cpp
        renderer/atmosphere/Transmittance.h
        renderer/atmosphere/MultipleScattering.cpp
        renderer/atmosphere/MultipleScattering.h
        renderer/atmosphere/SkyView.cpp
        renderer/atmosphere/SkyView.h
        renderer/depth/DepthPass.cpp
        renderer/depth/DepthPass.h
        renderer/atmosphere/AerialPerspective.cpp
        renderer/atmosphere/AerialPerspective.h
        renderer/composition/GodRaysPass.cpp
        renderer/composition/GodRaysPass.h
        renderer/Profiler.h
        renderer/BenchmarkResult.h

)

# Link the engine library
target_link_libraries(cloud_renderer PRIVATE hammock)
target_include_directories(cloud_renderer PRIVATE ${PROJECT_SOURCE_DIR}/include)