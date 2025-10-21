#pragma once

#include "hammock/core/Types.h"
#include "hammock/rendergraph/LogicalResource.h"

namespace Hammock {
    namespace Rendergraph {
        /// RenderPass interface
        /// Inherit from this interface to create a concrete render pass
        class ILogicalRenderPass {
           public:
            // This is called when pass is created, initialize all needed resources only in here
            virtual void onCreate() = 0;

            // Release all allocated resources here
            virtual void onRelease() = 0;

            struct CreateInfo {
                const CommandQueueFamily& commandQueueFamily;
                ResourceManager& resourceManager;
            };

            explicit ILogicalRenderPass(const CreateInfo& createInfo)
                : resourceManager(createInfo.resourceManager), commandQueueFamily(createInfo.commandQueueFamily) {
            }

            // Declare what resources this pass will read/write
            // Call read(...) to declare read access
            // Call write(...) to declare write access
            virtual void onDeclareResources() = 0;

            // Execute pass code in this method
            virtual void onRecordCommands(VkCommandBuffer) = 0;

            // Retrieves command queue family for this pass
            [[nodiscard]] CommandQueueFamily getCommandQueueFamily() const { return commandQueueFamily; }

           protected:
            // Declare read access
            void read(const LogicalResourceAccess& resourceAccess) {
                resourceReads.push_back(resourceAccess);
            }

            // Declare write access
            void write(const LogicalResourceAccess& resourceAccess) {
                resourceWrites.push_back(resourceAccess);
            }

            CommandQueueFamily commandQueueFamily;
            ResourceManager& resourceManager;
            std::vector<LogicalResourceAccess> resourceReads;
            std::vector<LogicalResourceAccess> resourceWrites;
        };

        class LogicalPassBuilder {
           public:
            using CreateCallback = std::function<void()>;
            using ReleaseCallback = std::function<void()>;
            using DeclareResourcesCallback = std::function<void(ILogicalRenderPass&)>;
            using RecordCommandsCallback = std::function<void(VkCommandBuffer)>;

            static LogicalPassBuilder create(const CommandQueueFamily& queueFamily, ResourceManager& rm) {
                return LogicalPassBuilder(queueFamily, rm);
            }

            LogicalPassBuilder& onCreate(CreateCallback cb) {
                onCreateCb = std::move(cb);
                return *this;
            }
            LogicalPassBuilder& onRelease(ReleaseCallback cb) {
                onReleaseCb = std::move(cb);
                return *this;
            }
            LogicalPassBuilder& onDeclareResources(DeclareResourcesCallback cb) {
                onDeclareResourcesCb = std::move(cb);
                return *this;
            }
            LogicalPassBuilder& onRecordCommands(RecordCommandsCallback cb) {
                onRecordCommandsCb = std::move(cb);
                return *this;
            }

            std::unique_ptr<ILogicalRenderPass> build(){
                struct LambdaPass : ILogicalRenderPass {
                    LambdaPass(const CreateInfo& info,
                               CreateCallback onCreate,
                               ReleaseCallback onRelease,
                               DeclareResourcesCallback onDeclareResources,
                               RecordCommandsCallback onRecordCommands)
                        : ILogicalRenderPass(info),
                          onCreateCb(std::move(onCreate)),
                          onReleaseCb(std::move(onRelease)),
                          onDeclareResourcesCb(std::move(onDeclareResources)),
                          onRecordCommandsCb(std::move(onRecordCommands)) {}

                    void onCreate() override {
                        if (onCreateCb) onCreateCb();
                    }
                    void onRelease() override {
                        if (onReleaseCb) onReleaseCb();
                    }
                    void onDeclareResources() override {
                        if (onDeclareResourcesCb) onDeclareResourcesCb(*this);
                    }
                    void onRecordCommands(VkCommandBuffer cmd) override {
                        if (onRecordCommandsCb) onRecordCommandsCb(cmd);
                    }

                    CreateCallback onCreateCb;
                    ReleaseCallback onReleaseCb;
                    DeclareResourcesCallback onDeclareResourcesCb;
                    RecordCommandsCallback onRecordCommandsCb;
                };

                ILogicalRenderPass::CreateInfo info{
                    .commandQueueFamily = queueFamily,
                    .resourceManager = resourceManager};

                return std::make_unique<LambdaPass>(
                    info,
                    std::move(onCreateCb),
                    std::move(onReleaseCb),
                    std::move(onDeclareResourcesCb),
                    std::move(onRecordCommandsCb));
            }

           private:
            LogicalPassBuilder(const CommandQueueFamily& qf, ResourceManager& rm)
                : queueFamily(qf), resourceManager(rm) {}

            CommandQueueFamily queueFamily;
            ResourceManager& resourceManager;
            CreateCallback onCreateCb;
            ReleaseCallback onReleaseCb;
            DeclareResourcesCallback onDeclareResourcesCb;
            RecordCommandsCallback onRecordCommandsCb;
        };
    }  // namespace Rendergraph
};  // namespace Hammock
