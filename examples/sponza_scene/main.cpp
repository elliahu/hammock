#include <print>
#include <cstdlib>
#include <memory>

import hammock.renderer.spirv_reflection;
import hammock.renderer.renderer;

int main() {
    try {
        auto reflection = std::make_unique<hammock::renderer::reflection::SpirvReflection>(
        "C:/dev/hammock/build-visual-studio/debug/spv/clouds.comp.spv");

        // Descriptor bindings
        auto bindings = reflection->getDescriptorBindings();

        // Input variables
        auto inputVariables = reflection->getInputInterfaceVariables();

        // Output variables
        auto outputVariables = reflection->getOutputInterfaceVariables();

        // Push blocks
        auto pushBlocks = reflection->getPushConstantBlocks();

        // Block fields
        auto fields = reflection->getPushConstantFields(*pushBlocks[0]);

    } catch(std::exception &e) {
        std::println("{}", e.what());
        exit(EXIT_FAILURE);
    }


    return EXIT_SUCCESS;
}
