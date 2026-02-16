#include <print>
#include <memory>
#include <stdexcept>

#include "renderer/gltf_loader.hpp"
#include "app/application.hpp"
#include "stage.hpp"

using namespace hammock;

int main() {
    auto app = std::make_unique<app::Application>(app::RunnerMode::Tooling);

    auto loader = std::make_unique<renderer::GltfLoader>(app->getVulkanContext());
    auto stage = std::make_unique<renderer::Stage>();
    try {
        loader->load("../../../data/scenes/cube_and_light/cube_and_light.glb", *stage);
    } catch (std::runtime_error err){
        std::println("Error loading glTF file: {}", err.what());
    }

    app->launch();

    return EXIT_SUCCESS;
}
