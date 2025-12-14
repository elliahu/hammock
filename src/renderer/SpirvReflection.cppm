module;

#include "spirv_reflect/spirv_reflect.h"
#include <stdexcept>
#include <vector>
#include <string>
#include <cstdint>

export module hammock.renderer.spirv_reflection;

import hammock.renderer.filesystem;

// TODO abstract SPV_REFLECT types and variables and don't export them outside of this module
namespace hammock::renderer::reflection {

    export using DescriptorBinding = ::SpvReflectDescriptorBinding;

    export class SpirvReflection {
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


        [[nodiscard]] std::vector<DescriptorBinding *> getDescriptorBindings() const {
            // Load
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
