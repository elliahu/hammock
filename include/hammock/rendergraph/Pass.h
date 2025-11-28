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
#include "hammock/core/HandmadeMath.h"
#include "hammock/core/ResourceManager.h"
#include "hammock/core/Types.h"
#include "hammock/rendergraph/ExecutionContext.h"
#include "hammock/rendergraph/Node.h"
#include "hammock/rendergraph/Resource.h"
#include "hammock/rendergraph/FrameGraphNodeHandle.h"

namespace Hammock {
    namespace Rendergraph {

        class Pass;

        enum class PassType { Graphics, Compute, Transfer };

        typedef std::function<void(ExecutionContext&)> ExecutionCallback;

        typedef Pass* PassPtr;

        struct ShaderInfo {
            const std::vector<char>& spv{};
        };

        struct PassResourceBindingInfo {
            FrameGraphNodeHandle resource;
            uint32_t binding;
        };

        struct ColorTargetInfo {
            FrameGraphNodeHandle image;
            VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        };

        struct DepthStencilInfo {
            FrameGraphNodeHandle image;
            VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        };

        struct PushConstantsInfo {
            size_t size = 0;
            const void* data = nullptr;
            VkShaderStageFlags stageFlags = VK_SHADER_STAGE_ALL;
        };

        struct UniformBufferInfo {
            FrameGraphNodeHandle buffer;
            VkShaderStageFlags stageFlags = VK_SHADER_STAGE_ALL;
        };

        struct StorageBufferInfo {
            FrameGraphNodeHandle buffer;
            VkShaderStageFlags stageFlags = VK_SHADER_STAGE_ALL;
        };

        struct StorageImageInfo {
            FrameGraphNodeHandle image;
            VkShaderStageFlags stageFlags = VK_SHADER_STAGE_ALL;
        };

        struct CombinedImageSamplerInfo {
            FrameGraphNodeHandle image;
            VkShaderStageFlags stageFlags = VK_SHADER_STAGE_ALL;
        };

        struct DispatchInfo {
            HmckVec3 size;
        };

        /// Pass definition
        class Pass : public Node {
           public:
            Pass(PassType type, const std::string& name) : Node(name), type(type) {}
            virtual ~Pass() = default;

            Pass(Pass&&) noexcept = default;  // enable moving
            Pass& operator=(Pass&&) noexcept = default;

            auto getType() -> PassType { return type; }

            /// Make this pass READ small constant values.
            /// Max size allowed: 128 bytes
            auto constants(const PushConstantsInfo& constants) {
                this->pushConstants = constants;
                hashPushConstants_ = true;
            }

            /// Make this pass READ from unform buffer.
            auto uniformBuffer(const UniformBufferInfo& uniformBufferInfo) {
                this->uniformBuffers.push_back(uniformBufferInfo);
            }

            /// Make this pass READ and WRITE into storage buffer.
            auto storageBuffer(const StorageBufferInfo& storageBufferInfo) {
                this->storageBuffers.push_back(storageBufferInfo);
            }

            /// Make this pass READ and WRITE into storage image.
            auto storageImage(const StorageImageInfo& storageImageInfo) {
                this->storageImages.push_back(storageImageInfo);
            }

            /// Make this pass READ from image
            auto combinedImageSampler(const CombinedImageSamplerInfo& imageSampler) {
                this->combinedImageSamplers.push_back(imageSampler);
            }

            /// Define descriptor bindings for this pass
            auto binding(std::initializer_list<PassResourceBindingInfo> bindingsList) {
                for (auto& binding : bindingsList) {
                    bindings.push_back(binding);
                }
            }

            auto hashPushConstants() { return hashPushConstants_; }
            auto getPushConstants() { return pushConstants; }
            auto getUniformBuffers() { return uniformBuffers; }
            auto getStorageBuffers() { return storageBuffers; }
            auto getStorageImages() { return storageImages; }
            auto getCombinedImageSamplers() { return combinedImageSamplers; }
            auto getDescriptorBindings() { return bindings; }

           private:
            PassType type;
            std::vector<PassResourceBindingInfo> bindings{};
            bool hashPushConstants_ = false;
            PushConstantsInfo pushConstants;
            std::vector<UniformBufferInfo> uniformBuffers{};
            std::vector<StorageBufferInfo> storageBuffers{};
            std::vector<StorageImageInfo> storageImages{};
            std::vector<CombinedImageSamplerInfo> combinedImageSamplers{};
        };

        /// Pass specializations
        class ComputePass : public Pass {
           private:
            std::unique_ptr<ShaderInfo> cs;
            DispatchInfo dispatchInfo;

           public:
            ComputePass(const std::string& name) : Pass(PassType::Compute, name) {}
            static auto create(const std::string& name) -> std::shared_ptr<ComputePass> {
                return std::move(std::make_shared<ComputePass>(name));
            }

            auto comp(ShaderInfo cs) { this->cs = std::make_unique<ShaderInfo>(std::move(cs)); }

            auto dispatch(const DispatchInfo& dispatchInfo) { this->dispatchInfo = dispatchInfo; };

            auto getDispatchInfo() { return dispatchInfo; }
        };

        class GraphicsPass : public Pass {
           private:
            std::unique_ptr<ShaderInfo> vs;
            std::unique_ptr<ShaderInfo> fs;
            ExecutionCallback exec{nullptr};
            std::vector<ColorTargetInfo> colorTargets{};
            DepthStencilInfo depthStencil_;
            bool hasDepthStencil_ = false;

           public:
            GraphicsPass(const std::string& name) : Pass(PassType::Graphics, name) {}

            static auto create(const std::string& name) -> std::shared_ptr<GraphicsPass> {
                return std::move(std::make_shared<GraphicsPass>(name));
            }

            auto colorTarget(const ColorTargetInfo& targets) { this->colorTargets.push_back(targets); }

            auto depthStencil(const DepthStencilInfo& depthStencil) {
                this->depthStencil_ = depthStencil;
                hasDepthStencil_ = true;
            }

            auto vert(ShaderInfo vs) { this->vs = std::make_unique<ShaderInfo>(std::move(vs)); }

            auto frag(ShaderInfo fs) { this->fs = std::make_unique<ShaderInfo>(std::move(fs)); }

            auto custom(ExecutionCallback callback) { exec = std::move(callback); }

            auto viewport(HmckVec2 pos, HmckVec2 size, HmckVec2 depth) -> void {};
            auto scissors(HmckVec2 offset, HmckVec2 extent) -> void {};

            auto getColorTargets() { return colorTargets; }
            auto hasDepthStencil() { return hasDepthStencil_; }
            auto getDepthStencil() { return depthStencil_; }
        };

    }  // namespace Rendergraph
};  // namespace Hammock
