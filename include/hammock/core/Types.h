#pragma once
#include <variant>
#include "hammock/core/Device.h"
#include "hammock/core/CoreUtils.h"

namespace hammock {
    namespace experimental {
        // Define resource types enum
        enum class ResourceType : uint32_t {
            Invalid = 0,
            Image,
            Buffer,
            MaxTypes
        };

        // Handle type that stores both resource type and ID
        class ResourceHandle {
        private:
            static constexpr uint64_t TYPE_SHIFT = 56;
            static constexpr uint64_t INDEX_MASK = (1ULL << TYPE_SHIFT) - 1;
            uint64_t packed_handle;

        public:
            ResourceHandle() : packed_handle(0) {
            }

            // Allow implicit conversion from existing ResourceHandle(uint64_t) constructor
            ResourceHandle(uint64_t id) : packed_handle(id & INDEX_MASK) {
            }

            static ResourceHandle create(ResourceType type, uint64_t resource_id) {
                ResourceHandle handle;
                handle.packed_handle = (static_cast<uint64_t>(type) << TYPE_SHIFT) | (resource_id & INDEX_MASK);
                return handle;
            }

            ResourceType getType() const {
                return static_cast<ResourceType>(packed_handle >> TYPE_SHIFT);
            }

            uint64_t getUid() const {
                return packed_handle & INDEX_MASK;
            }

            bool isValid() const {
                return packed_handle != 0 && getType() != ResourceType::Invalid;
            }

            bool operator==(const ResourceHandle &other) const {
                return packed_handle == other.packed_handle;
            }

            bool operator!=(const ResourceHandle &other) const {
                return packed_handle != other.packed_handle;
            }

            // TODO Helper for debugging
            const char *getTypeName() const {
                switch (getType()) {
                    default: return "Invalid";
                }
            }
        };

        // Type mapping traits
        template<typename T>
        struct ResourceTypeTraits {
            static constexpr ResourceType type = ResourceType::Invalid;
        };


        // Base resource class
        class Resource : public NonCopyable {
            friend struct ResourceFactory;

        protected:
            Device &device;
            uint64_t uid;
            std::string debug_name;
            VkDeviceSize size = 0;
            bool resident; // Whether the resource is currently in GPU memory

            Resource(Device &device, uint64_t uid, const std::string &name)
                : uid(uid), debug_name(name), resident(false), device(device) {
            }

        public:
            virtual ~Resource() = 0;

            virtual void load() = 0;

            virtual void unload() = 0;

            uint64_t getUid() const { return uid; }
            const std::string &getName() const { return debug_name; }
            bool isResident() const { return resident; }
            VkDeviceSize getSize() const { return size; } // for now
        };
    }

    /**
    * Describes general buffer
    */
    struct BufferDesc {
        VkDeviceSize instanceSize;
        uint32_t instanceCount;
        VkBufferUsageFlags usageFlags;
        VkMemoryPropertyFlags memoryPropertyFlags;
        VkDeviceSize minOffsetAlignment;
    };

    /**
    * Reference to Vulkan buffer.
    */
    struct BufferResourceRef {
        VkBuffer buffer;
        VmaAllocation allocation;
    };

    /**
     * Describes the base for the relative size
     */
    enum class RelativeSize {
        SwapChainRelative,
        FrameBufferRelative,
    };

    /**
    * Describes general image.
    */
    struct ImageDesc {
        uint32_t width, height, channels = 4, depth = 1, layers = 1, mips = 1;
        VkFormat format;
        VkImageUsageFlags usage;
        VkImageType imageType = VK_IMAGE_TYPE_2D;
        VkImageViewType imageViewType = VK_IMAGE_VIEW_TYPE_2D;
        VkClearValue clearValue = {};
        bool createSampler = true;
        VkFilter filter = VK_FILTER_LINEAR;
        VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        VkBorderColor borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        VkSamplerMipmapMode mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        VkSampleCountFlagBits imageSampleCount = VK_SAMPLE_COUNT_1_BIT;
    };

    /**
     * Reference to Vulkan image.
     */
    struct ImageResourceRef {
        VkImage image;
        VkImageView view;
        VkSampler sampler;
        VmaAllocation allocation;
        VkImageLayout currentLayout;
        VkClearValue clearValue = {0.0f, 0.0f, 0.2f, 1.0f};
    };


    /**
     * Resource reference. It can hold a reference to image, or buffer.
     */
    struct ResourceRef {
        // Variant to hold either buffer or image resource
        std::variant<BufferResourceRef, ImageResourceRef> resource;
    };
}
