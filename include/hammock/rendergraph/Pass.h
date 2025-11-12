#pragma once

#include <functional>
#include <initializer_list>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "hammock/core/ComputePipeline.h"
#include "hammock/core/CoreUtils.h"
#include "hammock/core/FrameManager.h"
#include "hammock/core/GraphicsPipeline.h"
#include "hammock/core/ResourceManager.h"
#include "hammock/core/Types.h"
#include "hammock/rendergraph/ExecutionContext.h"
#include "hammock/rendergraph/Node.h"
#include "hammock/rendergraph/Resource.h"

namespace Hammock {
    namespace Rendergraph {

        class Pass;

        enum class PassType { Graphics, Compute, Transfer };

        typedef std::function<void(ExecutionContext&)> ExecutionCallback;

        typedef Pass* PassPtr;

        struct PassResourceBinding {
            uint32_hash_t resource;
            uint32_t binding;
            VkDescriptorType descriptorType;
            VkShaderStageFlags stageFlags;
            uint32_t count = 1;
            VkDescriptorBindingFlags flags = 0;
        };

        /// Pass definition
        class Pass : public Node {
           public:
            Pass(PassType type, const std::string& name) : Node(HashName(name.c_str())), type(type) {}
            virtual ~Pass() = default;

            Pass(Pass&&) noexcept = default;  // enable moving
            Pass& operator=(Pass&&) noexcept = default;

            auto getType() -> PassType { return type; }

            auto read(std::initializer_list<uint32_hash_t> handles) -> Pass& {
                for (const auto& handle : handles) {
                    to.push_back(handle);
                }
                return *this;
            }

            auto write(std::initializer_list<uint32_hash_t> handles) -> Pass& {
                for (const auto& handle : handles) {
                    from.push_back(handle);
                }
                return *this;
            }

            auto bind(std::initializer_list<PassResourceBinding> bindingsList) -> Pass& {
                for (auto& binding : bindingsList) {
                    bindings.push_back(binding);
                }
                return *this;
            }

            auto execute(ExecutionCallback callback) -> Pass& {
                exec = std::move(callback);
                return *this;
            }

            auto compute(ShaderModule cs) -> Pass& {
                this->cs = std::make_unique<ShaderModule>(std::move(cs));
                return *this;
            }

            auto vertex(ShaderModule vs) -> Pass& {
                this->vs = std::make_unique<ShaderModule>(std::move(vs));
                return *this;
            }

            auto fragment(ShaderModule fs) -> Pass& {
                this->fs = std::make_unique<ShaderModule>(std::move(fs));
                return *this;
            }

            std::unique_ptr<ShaderModule> cs;
            std::unique_ptr<ShaderModule> vs;
            std::unique_ptr<ShaderModule> fs;

            ExecutionCallback exec{nullptr};
            PassType type;
            std::vector<PassResourceBinding> bindings{};
        };

        /// Pass specializations
        class ComputePass : public Pass {
           public:
            ComputePass(const std::string& name) : Pass(PassType::Compute, name) {}
        };

        class GraphicsPass : public Pass {
           public:
            GraphicsPass(const std::string& name) : Pass(PassType::Graphics, name) {}
        };

        class TransferPass : public Pass {
           public:
            TransferPass(const std::string& name) : Pass(PassType::Transfer, name) {}
        };

    }  // namespace Rendergraph
};  // namespace Hammock
