#include <print>
#include <cstdlib>
#include <memory>

import hammock.renderer.spirv_reflection;

int main() {
    try {
        auto reflection = std::make_unique<hammock::renderer::reflection::SpirvReflection>(
        "C:/dev/hammock/build-visual-studio/debug/spv/clouds.comp.spv");

        // Descriptor bindings
        auto bindings = reflection->getDescriptorBindings();

    } catch(std::exception &e) {
        std::println("{}", e.what());
        exit(EXIT_FAILURE);
    }


    return EXIT_SUCCESS;
}
