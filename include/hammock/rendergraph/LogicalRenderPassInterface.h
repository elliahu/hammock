#pragma once

#include <string>

#include "hammock/core/FrameManager.h"
#include "hammock/core/ResourceManager.h"
#include "hammock/core/Types.h"
#include "hammock/rendergraph/LogicalResource.h"
#include "hammock/rendergraph/NodeInterface.h"

namespace Hammock {
    namespace Rendergraph {
        /// RenderPass interface
        /// Inherit from this interface to create a concrete render pass
        class LogicalRenderPassInterface : public NodeInterface {
           public:
            // This is called when pass is created, initialize all needed resources only in here
            virtual void onCreate() = 0;

            // Release all allocated resources here
            virtual void onRelease() = 0;

            struct CreateInfo {
                uint32_hash_t hashName;
                const CommandQueueFamily& commandQueueFamily;
                ResourceManager& resourceManager;
                FrameManager& frameManager;
                Device& device;
            };

            explicit LogicalRenderPassInterface(const CreateInfo& createInfo)
                : NodeInterface(createInfo.hashName),
                  resourceManager(createInfo.resourceManager),
                  commandQueueFamily(createInfo.commandQueueFamily),
                  frameManager(createInfo.frameManager),
                  device(createInfo.device) {}

            virtual ~LogicalRenderPassInterface() = default;

            // Execute pass code in this method
            virtual void onRecordCommands(VkCommandBuffer) = 0;

            // Retrieves command queue family for this pass
            [[nodiscard]] CommandQueueFamily getCommandQueueFamily() const { return commandQueueFamily; }

            // Declare read access
            void read(uint32_t resourceHashName) { to.push_back(resourceHashName); }

            // Declare write access
            void write(uint32_t resourceHashName) { from.push_back(resourceHashName); }

           protected:
            CommandQueueFamily commandQueueFamily;
            ResourceManager& resourceManager;
            FrameManager& frameManager;
            Device& device;
        };

        class LogicalPassBuilder {
           public:
            using CreateCallback = std::function<void()>;
            using ReleaseCallback = std::function<void()>;
            using RecordCommandsCallback = std::function<void(VkCommandBuffer)>;

            static LogicalPassBuilder create(
                uint32_hash_t hashName, const CommandQueueFamily& queueFamily, ResourceManager& rm, FrameManager& fm, Device& dv) {
                return LogicalPassBuilder(hashName, queueFamily, rm, fm, dv);
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
                to.push_back(resourceHashName);
                return *this;
            }

            LogicalPassBuilder& write(uint32_t resourceHashName) {
                from.push_back(resourceHashName);
                return *this;
            }
            // -------------------------------------------

            std::unique_ptr<LogicalRenderPassInterface> build() {
                struct LambdaPass : LogicalRenderPassInterface {
                    LambdaPass(const CreateInfo& info, CreateCallback onCreate, ReleaseCallback onRelease,
                        RecordCommandsCallback onRecordCommands, std::vector<uint32_t> reads,
                        std::vector<uint32_t> writes)
                        : LogicalRenderPassInterface(info),
                          onCreateCb(std::move(onCreate)),
                          onReleaseCb(std::move(onRelease)),
                          onRecordCommandsCb(std::move(onRecordCommands)) {
                        // Transfer resource declarations
                        to = std::move(reads);
                        from = std::move(writes);
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

                LogicalRenderPassInterface::CreateInfo info{
                    .hashName = hashName,
                    .commandQueueFamily = queueFamily,
                    .resourceManager = resourceManager,
                    .frameManager = frameManager,
                    .device = device
                };

                return std::make_unique<LambdaPass>(info,
                    std::move(onCreateCb),
                    std::move(onReleaseCb),
                    std::move(onRecordCommandsCb),
                    std::move(to),
                    std::move(from));
            }

           private:
            LogicalPassBuilder(
                uint32_hash_t hashName, const CommandQueueFamily& qf, ResourceManager& rm, FrameManager& fm, Device& dv)
                : hashName(hashName), queueFamily(qf), resourceManager(rm), frameManager(fm), device(dv) {}

            uint32_hash_t hashName;
            CommandQueueFamily queueFamily;
            ResourceManager& resourceManager;
            FrameManager& frameManager;
            Device& device;
            CreateCallback onCreateCb;
            ReleaseCallback onReleaseCb;
            RecordCommandsCallback onRecordCommandsCb;

            std::vector<uint32_t> to;
            std::vector<uint32_t> from;
        };

    }  // namespace Rendergraph
};  // namespace Hammock
