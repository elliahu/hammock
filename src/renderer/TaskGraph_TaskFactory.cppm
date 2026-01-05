module;

#include <string>
#include <memory>
#include <stdexcept>

export module hammock.renderer.task_graph:task_factory;
import :task;
import hammock.renderer.filesystem;
import hammock.renderer.spirv_reflection;

namespace hammock::renderer {
    /// @class TaskFactory
    /// @brief Helper factory class to create tasks from shader files using reflectionSpirvReflection.
    export class TaskFactory final {
    public:
        /// @brief Creates ComputeTask from SpirV shader
        /// @param computeShaderFilename filename of the compute shader
        static std::unique_ptr<ComputeTask> createComputeTask(const std::string &computeShaderFilename) {
            auto shader = filesystem::readFile(computeShaderFilename);
            auto reflection = std::make_unique<reflection::SpirvReflection>(shader);

            // Prepare the task
            std::unique_ptr<ComputeTask> task = std::make_unique<ComputeTask>(computeShaderFilename);

            // Inspect bindings
            auto bindings = reflection->getDescriptorBindings();
            for (auto &binding: bindings) {
                // Prepare the socket
                std::unique_ptr<ISocket> socket;
                std::uint32_t arrayCount = deduceArrayCount(binding);
                std::string name = binding->name;
                DescriptorBinding descBinding{
                    .set = binding->set,
                    .binding = binding->binding,
                    .usage = ComputeShader,
                    .count = arrayCount,
                };


                switch (binding->descriptor_type) {
                    case reflection::SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                        socket = std::make_unique<ImageSocket>(name, ImageState::SampledRead, deduceImageType(binding),
                                                               descBinding);
                        break;
                    case reflection::SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
                        socket = std::make_unique<ImageSocket>(name, ImageState::StorageReadWrite,
                                                               deduceImageType(binding),
                                                               descBinding);
                        break;
                    case reflection::SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                        socket = std::make_unique<BufferSocket>(name, BufferState::UniformBufferRead,
                                                                descBinding);
                        break;
                    case reflection::SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
                        socket = std::make_unique<BufferSocket>(name, BufferState::StorageBufferReadWrite,
                                                                 descBinding);
                        break;
                    case reflection::SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
                    case reflection::SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
                    case reflection::SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                    case reflection::SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
                    case reflection::SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
                    case reflection::SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
                    case reflection::SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
                    case reflection::SpvReflectDescriptorType::SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
                    default:
                        throw std::runtime_error("unimplemented");
                }

                task->addSocket(std::move(socket));
            }

            // Push constants
            // TODO support multiple push blocks
            auto pushBlocks = reflection->getPushConstantBlocks();
            auto pushConstantBlock = std::make_unique<PushConstantsBlock>();
            auto fields = reflection->getPushConstantFields(*pushBlocks[0]);
            for (auto &f: fields) {
                auto field = std::make_unique<PushConstantField>(f->name, deducePushConstantFieldType(f));
                pushConstantBlock->addField(std::move(field));
            }
            task->addPushConstantBlock(std::move(pushConstantBlock));

            return task;
        }

        /// @brief Creates GraphicsTask from Spirv shaders
        /// @param vertexShaderFilename vertex shader filename
        /// @param fragmentShaderFilename fragment shader filename
        static std::unique_ptr<GraphicsTask> createGraphicsTask(const std::string &vertexShaderFilename,
                                                                const std::string &fragmentShaderFilename) {
            throw std::runtime_error("unimplemented");
        }

    private:
        // TODO implement
        static ImageType deduceImageType(reflection::SpvReflectDescriptorBinding *binding) {
            throw std::runtime_error("unimplemented");
        }

        // TODO support multidimensional arrays
        static std::uint32_t deduceArrayCount(reflection::SpvReflectDescriptorBinding *binding) {
            if (binding->array.dims_count > 0) {
                // No array
                return 1u;
            }

            if (binding->array.dims[0] == 0) {
                // Unsized array
                return 1u;
            }
            // Sized array
            return binding->array.dims[0];
        }

        // TODO implement
        static PushConstantFieldType deducePushConstantFieldType(const reflection::SpvReflectBlockVariable *&field) {
            throw std::runtime_error("unimplemented");
        }
    };
}
