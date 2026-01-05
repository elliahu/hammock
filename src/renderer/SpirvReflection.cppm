module;

#include "spirv_reflect/spirv_reflect.h"
#include <stdexcept>
#include <vector>
#include <string>
#include <cstdint>
#include <memory>
#include <functional>

export module hammock_renderer.spirv_reflection;

import hammock_renderer.filesystem;


// TODO abstract SPV_REFLECT types and variables and don't export them outside of this module
namespace hammock::renderer::reflection {
    export using ::SpvReflectDescriptorBinding;
    export using ::SpvReflectInterfaceVariable;
    export using ::SpvReflectDescriptorType;
    export using ::SpvDim;
    export using ::SpvReflectBlockVariable;

    /// @class SpirvReflection
    /// @brief Helper class for inspecting SPIR-V shader.
    /// You can use this to create you pipeline objects
    export class SpirvReflection final{
    public:
        explicit SpirvReflection(const std::string &filename) {
            try {
                auto buffer = filesystem::readFile(filename);
                createShaderModuleReflection(static_cast<const void *>(buffer.data()), buffer.size());
            } catch (std::exception &e) {
                throw std::runtime_error("SpirvReflection failed for file '" + filename + "': " + e.what());
            }
        }

        explicit SpirvReflection(const std::vector<char> &buffer) {
            try {
                createShaderModuleReflection(static_cast<const void *>(buffer.data()), buffer.size());
            } catch (std::exception &e) {
                throw std::runtime_error("SpirvReflection failed: " + std::string(e.what()));
            }
        }

        ~SpirvReflection() {
            spvReflectDestroyShaderModule(&module);
        }


        [[nodiscard]] std::vector<SpvReflectDescriptorBinding *> getDescriptorBindings() const {
            uint32_t count = 0;
            if (auto result = spvReflectEnumerateDescriptorBindings(&module, &count, nullptr);
                result != SPV_REFLECT_RESULT_SUCCESS) {
                throw std::runtime_error("failed to enumerate descriptor bindings, code: " + result);
            }

            std::vector<SpvReflectDescriptorBinding *> descriptorBindings(count);
            if (auto result = spvReflectEnumerateDescriptorBindings(&module, &count, descriptorBindings.data());
                result != SPV_REFLECT_RESULT_SUCCESS) {
                throw std::runtime_error("failed to enumerate descriptor bindings, code: " + result);
            }
            return descriptorBindings;
        }

        [[nodiscard]] std::vector<SpvReflectInterfaceVariable *> getInputInterfaceVariables() const {
            uint32_t count = 0;
            if (auto result = spvReflectEnumerateInputVariables(&module, &count, nullptr);
                result != SPV_REFLECT_RESULT_SUCCESS) {
                throw std::runtime_error("failed to enumerate input variables, code: " + result);
            }

            std::vector<SpvReflectInterfaceVariable *> inputVariables(count);
            if (auto result = spvReflectEnumerateInputVariables(&module, &count, inputVariables.data());
                result != SPV_REFLECT_RESULT_SUCCESS) {
                throw std::runtime_error("failed to enumerate input variables, code: " + result);
            }

            return inputVariables;
        }

        [[nodiscard]] std::vector<SpvReflectInterfaceVariable *> getOutputInterfaceVariables() const {
            uint32_t count = 0;
            if (auto result = spvReflectEnumerateOutputVariables(&module, &count, nullptr);
                result != SPV_REFLECT_RESULT_SUCCESS) {
                throw std::runtime_error("failed to enumerate output variables, code: " + result);
            }

            std::vector<SpvReflectInterfaceVariable *> inputVariables(count);
            if (auto result = spvReflectEnumerateOutputVariables(&module, &count, inputVariables.data());
                result != SPV_REFLECT_RESULT_SUCCESS) {
                throw std::runtime_error("failed to enumerate output variables, code: " + result);
            }

            return inputVariables;
        }

        [[nodiscard]] std::vector<SpvReflectBlockVariable *> getPushConstantBlocks() const {
            uint32_t count = 0;
            if (auto result = spvReflectEnumeratePushConstantBlocks(&module, &count, nullptr);
                result != SPV_REFLECT_RESULT_SUCCESS) {
                throw std::runtime_error("failed to enumerate push constants, code: " + result);
            }

            std::vector<SpvReflectBlockVariable *> pushConstantBlocks(count);
            if (auto result = spvReflectEnumeratePushConstantBlocks(&module, &count, pushConstantBlocks.data());
                result != SPV_REFLECT_RESULT_SUCCESS) {
                throw std::runtime_error("failed to enumerate push constants, code: " + result);
            }

            return pushConstantBlocks;
        }

        static std::vector<const SpvReflectBlockVariable *> getPushConstantFields(
            const SpvReflectBlockVariable &block) {
            std::vector<const SpvReflectBlockVariable *> fields;

            std::function<void(const SpvReflectBlockVariable &)> visit =
                    [&](const SpvReflectBlockVariable &var) {
                for (uint32_t i = 0; i < var.member_count; ++i) {
                    const SpvReflectBlockVariable &member = var.members[i];

                    if (member.member_count > 0) {
                        // Nested struct
                        visit(member);
                    } else {
                        // Leaf field
                        fields.push_back(&member);
                    }
                }
            };

            visit(block);
            return fields;
        }

    private:
        SpvReflectShaderModule module{};

        void createShaderModuleReflection(const void *data, size_t bytes) {
            if (auto result = spvReflectCreateShaderModule(bytes, data, &module);
                result != SPV_REFLECT_RESULT_SUCCESS) {
                throw std::runtime_error("failed to create shader module, code: " + result);
            }
        }
    };

}
