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

        enum class PassType { Graphics, Compute, Transfer };

        typedef std::function<void(ExecutionContext&)> ExecutionCallback;

        class Pass : public Node {
           public:
            Pass(PassType type, uint32_hash_t hash) : Node(hash), type(type) {}
            virtual ~Pass() = default;

            auto getType() -> PassType { return type; }

            std::unordered_map<uint32_hash_t, uint32_t> bindings{};

            ExecutionCallback execute{nullptr};

           private:
            PassType type;
        };

        class ComputePass : public Pass {
           public:
            ComputePass(PassType type, uint32_hash_t hash) : Pass(type, hash) {}
            std::unique_ptr<ComputePipeline> pipeline;
        };

        class GraphicsPass : public Pass {
           public:
            GraphicsPass(PassType type, uint32_hash_t hash) : Pass(type, hash) {}
            std::unique_ptr<GraphicsPipeline> pipeline;
        };

        class TransferPass : public Pass {
           public:
            TransferPass(PassType type, uint32_hash_t hash) : Pass(type, hash) {}
        };

        template <typename PassT>
        class PassBuilder {
            static_assert(std::is_base_of<Pass, PassT>::value, "PassT must derive from Pass");

           private:
            std::unique_ptr<PassT> pass;

           public:
            explicit PassBuilder(uint32_hash_t hash) : pass(std::make_unique<PassT>(hash)) {}

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

            PassBuilder& bindings(std::initializer_list<std::pair<uint32_hash_t, uint32_t>> bindingsList) {
                for (auto& binding : bindingsList) {
                    pass->bindings[binding.first] = binding.second;
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
