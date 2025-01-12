#pragma once
#include <string>
#include <vulkan/vulkan.h>

#include "hammock/core/Resource.h"

namespace hammock {
    namespace rendergraph {
        struct ImageDescription {
            uint32_t width = 0, height = 0, channels = 0, depth = 1, layers = 1, mips = 1;
            VkImageType imageType = VK_IMAGE_TYPE_2D;
            VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
            VkFormat format = VK_FORMAT_UNDEFINED;
            VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        };

        class Image : public Resource {
            friend struct ResourceFactory;
        protected:
            Image(Device &device, uint64_t uid, const std::string &name, const ImageDescription &info): Resource(
                    device, uid, name), desc(info) {
            }

            VkImage image = VK_NULL_HANDLE;
            VkImageView view = VK_NULL_HANDLE;
            VmaAllocation allocation = VK_NULL_HANDLE;
            ImageDescription desc;

        public:
            void load() override;;

            void unload() override;
        };


        struct FramebufferAttachmentDescription : ImageDescription {
            VkAttachmentLoadOp loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        };

        class FramebufferAttachment : public Image {
            friend struct ResourceFactory;
        protected:
            explicit FramebufferAttachment(Device &device, uint64_t uid, const std::string &name,
                                           const FramebufferAttachmentDescription &info) : Image(
                    device, uid, name, {
                        .width = info.width,
                        .height = info.height,
                        .channels = info.channels,
                        .depth = info.depth,
                        .layers = info.layers,
                        .mips = info.mips,
                        .layout = info.layout,
                        .format = info.format,
                        .usage = info.usage,
                    }), desc(info) {
            }

            FramebufferAttachmentDescription desc;

        public:
        };


        struct SampledImageDescription : ImageDescription {
            VkFilter filter = VK_FILTER_LINEAR;
            VkSampleCountFlagBits imageSampleCount = VK_SAMPLE_COUNT_1_BIT;
        };

        class SampledImage : public Image {
            friend struct ResourceFactory;
        protected:
            explicit SampledImage(Device &device, uint64_t uid, const std::string &name,
                                  const SampledImageDescription &info) : Image(
                                                                      device, uid, name, {
                                                                          .width = info.width,
                                                                          .height = info.height,
                                                                          .channels = info.channels,
                                                                          .depth = info.depth,
                                                                          .layers = info.layers,
                                                                          .mips = info.mips,
                                                                          .imageType = info.imageType,
                                                                          .layout = info.layout,
                                                                          .format = info.format,
                                                                          .usage = info.usage,
                                                                      }), desc(info) {
            }

            VkSampler sampler = VK_NULL_HANDLE;
            SampledImageDescription desc;

        public:
            void load() override;;

            void unload() override;
        };
    }
}