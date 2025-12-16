module;

#include <string>
#include <memory>
#include <stdexcept>

export module hammock.renderer.task_graph:task_factory;
import :task;

namespace hammock::renderer {
    /// @class TaskFactory
    /// @brief Helper factory class to create sockets from shader files using reflectionSpirvReflection.
    export class TaskFactory final {
    public:
        /// @brief Creates ComputeTask from SpirV shader
        /// @param computeShaderFilename filename of the compute shader
        static std::unique_ptr<ComputeTask> createComputeTask(const std::string &computeShaderFilename) {
            throw std::exception("unimplemented");
        }

        /// @brief Creates GraphicsTask from Spirv shaders
        /// @param vertexShaderFilename vertex shader filename
        /// @param fragmentShaderFilename fragment shader filename
        static std::unique_ptr<GraphicsTask> createGraphicsTask(const std::string &vertexShaderFilename,
                                                                const std::string &fragmentShaderFilename) {
            throw std::exception("unimplemented");
        }

    private:
    };
}