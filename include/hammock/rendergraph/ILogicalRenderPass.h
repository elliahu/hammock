#pragma once

#include <string>

#include "hammock/core/Types.h"
#include "hammock/rendergraph/LogicalResource.h"

namespace Hammock {
    namespace Rendergraph {
        /// RenderPass interface
        /// Inherit from this interface to create a concrete render pass
        class ILogicalRenderPass {
           public:
            std::vector<uint32_hash_t> resourceReads; // resources read by this pass
            std::vector<uint32_hash_t> resourceWrites; // resources written by this pass
            std::vector<uint32_t> incomingEdges; // indices of edges coming into this pass
            std::vector<uint32_t> outgoingEdges; // indices of edges going out from this pass
            // This is called when pass is created, initialize all needed resources only in here
            virtual void onCreate() = 0;

            // Release all allocated resources here
            virtual void onRelease() = 0;

            struct CreateInfo {
                uint32_hash_t hashName;
                const CommandQueueFamily& commandQueueFamily;
                ResourceManager& resourceManager;
            };

            explicit ILogicalRenderPass(const CreateInfo& createInfo)
                : hashName(createInfo.hashName), resourceManager(createInfo.resourceManager), commandQueueFamily(createInfo.commandQueueFamily) {
            }

            // Execute pass code in this method
            virtual void onRecordCommands(VkCommandBuffer) = 0;

            // Retrieves command queue family for this pass
            [[nodiscard]] CommandQueueFamily getCommandQueueFamily() const { return commandQueueFamily; }

            // Declare read access
            void read(uint32_t resourceHashName) {
                resourceReads.push_back(resourceHashName);
            }

            // Declare write access
            void write(uint32_t resourceHashName) {
                resourceWrites.push_back(resourceHashName);
            }

            [[nodiscard]] uint32_hash_t getHashName() const { return hashName; }

           protected:
            uint32_hash_t hashName;
            CommandQueueFamily commandQueueFamily;
            ResourceManager& resourceManager;
        };

        class LogicalPassBuilder {
           public:
            using CreateCallback = std::function<void()>;
            using ReleaseCallback = std::function<void()>;
            using RecordCommandsCallback = std::function<void(VkCommandBuffer)>;

            static LogicalPassBuilder create(uint32_hash_t hashName, const CommandQueueFamily& queueFamily, ResourceManager& rm) {
                return LogicalPassBuilder(hashName, queueFamily, rm);
            }

            LogicalPassBuilder& onCreate(CreateCallback cb) {
                onCreateCb = std::move(cb);
                return *this;
            }

            LogicalPassBuilder& onRelease(ReleaseCallback cb) {
                onReleaseCb = std::move(cb);
                return *this;
            }

            LogicalPassBuilder& onRecordCommands(RecordCommandsCallback cb) {
                onRecordCommandsCb = std::move(cb);
                return *this;
            }

            // --- New: resource declaration chaining ---
            LogicalPassBuilder& read(uint32_t resourceHashName) {
                resourceReads.push_back(resourceHashName);
                return *this;
            }

            LogicalPassBuilder& write(uint32_t resourceHashName) {
                resourceWrites.push_back(resourceHashName);
                return *this;
            }
            // -------------------------------------------

            std::unique_ptr<ILogicalRenderPass> build() {
                struct LambdaPass : ILogicalRenderPass {
                    LambdaPass(const CreateInfo& info,
                               CreateCallback onCreate,
                               ReleaseCallback onRelease,
                               RecordCommandsCallback onRecordCommands,
                               std::vector<uint32_t> reads,
                               std::vector<uint32_t> writes)
                        : ILogicalRenderPass(info),
                          onCreateCb(std::move(onCreate)),
                          onReleaseCb(std::move(onRelease)),
                          onRecordCommandsCb(std::move(onRecordCommands)) {
                        // Transfer resource declarations
                        resourceReads = std::move(reads);
                        resourceWrites = std::move(writes);
                    }

                    void onCreate() override {
                        if (onCreateCb) onCreateCb();
                    }

                    void onRelease() override {
                        if (onReleaseCb) onReleaseCb();
                    }

                    void onRecordCommands(VkCommandBuffer cmd) override {
                        if (onRecordCommandsCb) onRecordCommandsCb(cmd);
                    }

                    CreateCallback onCreateCb;
                    ReleaseCallback onReleaseCb;
                    RecordCommandsCallback onRecordCommandsCb;
                };

                ILogicalRenderPass::CreateInfo info{
                    .hashName = hashName,
                    .commandQueueFamily = queueFamily,
                    .resourceManager = resourceManager};

                return std::make_unique<LambdaPass>(
                    info,
                    std::move(onCreateCb),
                    std::move(onReleaseCb),
                    std::move(onRecordCommandsCb),
                    std::move(resourceReads),
                    std::move(resourceWrites));
            }

           private:
            LogicalPassBuilder(uint32_hash_t hashName, const CommandQueueFamily& qf, ResourceManager& rm)
                : hashName(hashName), queueFamily(qf), resourceManager(rm) {}

            uint32_hash_t hashName;
            CommandQueueFamily queueFamily;
            ResourceManager& resourceManager;
            CreateCallback onCreateCb;
            ReleaseCallback onReleaseCb;
            RecordCommandsCallback onRecordCommandsCb;

            std::vector<uint32_t> resourceReads; 
            std::vector<uint32_t> resourceWrites; 
        };

    }  // namespace Rendergraph
};  // namespace Hammock
