#pragma once

#include <functional>
#include <initializer_list>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include "hammock/core/ComputePipeline.h"
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

        class Pass : public Node {
           public:
            Pass(PassType type, const std::string& name) : Node(HashName(name.c_str())), type(type) {}
            virtual ~Pass() = default;

            auto getType() -> PassType { return type; }

            std::vector<PassResourceBinding> bindings{};

            ExecutionCallback execute{nullptr};

           private:
            PassType type;
        };

        class ComputePass : public Pass {
           public:
            ComputePass(const std::string& name) : Pass(PassType::Compute, name) {}
            std::unique_ptr<ComputePipeline> pipeline;
        };

        class GraphicsPass : public Pass {
           public:
            GraphicsPass(const std::string& name) : Pass(PassType::Graphics, name) {}
            std::unique_ptr<GraphicsPipeline> pipeline;
        };

        class TransferPass : public Pass {
           public:
            TransferPass(const std::string& name) : Pass(PassType::Transfer, name) {}
        };

        template <typename PassT>
        class PassBuilder {
            static_assert(std::is_base_of<Pass, PassT>::value, "PassT must derive from Pass");

           private:
            std::unique_ptr<PassT> pass;

           public:
            explicit PassBuilder(const std::string& name) : pass(std::make_unique<PassT>(name)) {}

            auto read(std::initializer_list<uint32_hash_t> handles) -> PassBuilder& {
                for (const auto& handle : handles) {
                    pass->to.push_back(handle);
                }
                return *this;
            }
            auto write(std::initializer_list<uint32_hash_t> handles) -> PassBuilder& {
                for (const auto& handle : handles) {
                    pass->from.push_back(handle);
                }
                return *this;
            }

            PassBuilder& bindings(std::initializer_list<PassResourceBinding> bindingsList) {
                for (auto& binding : bindingsList) {
                    pass->bindings.push_back(binding);
                }
                return *this;
            }

            PassBuilder& execute(ExecutionCallback callback) {
                pass->execute = std::move(callback);
                return *this;
            }

            // Graphics-specific
            template <typename T = PassT>
            std::enable_if_t<std::is_same_v<T, GraphicsPass>, PassBuilder&> pipeline(
                std::unique_ptr<GraphicsPipeline> p) {
                pass->pipeline = std::move(p);
                return *this;
            }

            // Compute-specific
            template <typename T = PassT>
            std::enable_if_t<std::is_same_v<T, ComputePass>, PassBuilder&> pipeline(
                std::unique_ptr<ComputePipeline> p) {
                pass->pipeline = std::move(p);
                return *this;
            }

            std::unique_ptr<PassT> build() { return std::move(pass); }
        };

    }  // namespace Rendergraph
};  // namespace Hammock
