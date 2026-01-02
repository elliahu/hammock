#include <print>
#include <cstdlib>
#include <memory>

import hammock.renderer.spirv_reflection;
import hammock.renderer.renderer;
import hammock.renderer.filesystem;
import hammock.renderer.task_graph;
import hammock.engine.engine;
import hammock.engine.editor;

using namespace hammock::renderer;

int main() {
    try {
        auto shader = hammock::renderer::filesystem::readFile("C:/dev/hammock/build-debug-clang/spv/clouds.comp.spv");
        auto reflection = std::make_unique<hammock::renderer::reflection::SpirvReflection>(shader);

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


        auto computeTask = std::make_unique<ComputeTask>("C:/dev/hammock/build-visual-studio/debug/spv/clouds.comp.spv");
        auto bufferSocket = std::make_unique<BufferSocket>("data", BufferState::UniformBufferRead, DescriptorBinding{0,0, ComputeShader});
        computeTask->addSocket(std::move(bufferSocket));
        auto pushBlock = std::make_unique<PushConstantsBlock>();
        auto field = std::make_unique<PushConstantField>("field", PushConstantFieldType::Float);
        pushBlock->addField(std::move(field))->setFloat(0.5f);
        computeTask->addPushConstantBlock(std::move(pushBlock));

        auto engine = std::make_unique<hammock::engine::Engine>(hammock::engine::LaunchMode::Runtime);
        engine->launch();

    } catch(std::exception &e) {
        std::println("{}", e.what());
        exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}
