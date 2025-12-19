module;
#include <vulkan/vulkan.h>
#include <string>
#include <cstdint>

export module hammock.core.base_resource;

import hammock.core.device;



namespace hammock::core {

    // Define resource types enum
    export enum class ResourceType : uint32_t { Invalid = 0, Image, Sampler, Buffer, MaxTypes };

    // Handle type that stores both resource type and ID
    export class ResourceHandle {
    private:
        static constexpr uint64_t TYPE_SHIFT = 56;
        static constexpr uint64_t INDEX_MASK = (1ULL << TYPE_SHIFT) - 1;
        uint64_t packed_handle;

    public:
        ResourceHandle() : packed_handle(0) {}

        // Allow implicit conversion from existing ResourceHandle(uint64_t) constructor
        ResourceHandle(uint64_t id) : packed_handle(id & INDEX_MASK) {}

        static ResourceHandle create(ResourceType type, uint64_t resource_id) {
            ResourceHandle handle;
            handle.packed_handle = (static_cast<uint64_t>(type) << TYPE_SHIFT) | (resource_id & INDEX_MASK);
            return handle;
        }

        [[nodiscard]] ResourceType getType() const { return static_cast<ResourceType>(packed_handle >> TYPE_SHIFT); }

        [[nodiscard]] uint64_t getUid() const { return packed_handle & INDEX_MASK; }

        bool isValid() const { return packed_handle != 0 && getType() != ResourceType::Invalid; }

        bool operator==(const ResourceHandle& other) const { return packed_handle == other.packed_handle; }

        bool operator!=(const ResourceHandle& other) const { return packed_handle != other.packed_handle; }
    };

    // Type mapping traits
    export template <typename T>
    struct ResourceTypeTraits {
        static constexpr ResourceType type = ResourceType::Invalid;
    };

    // Base resource class
    export class BaseResource {
    protected:
        Device& device;
        uint64_t uid;
        std::string debug_name;
        VkDeviceSize size = 0;
        bool resident;  // Whether the resource is currently in GPU memory

        BaseResource(Device& device, std::uint64_t uid, const std::string& name)
            : uid(uid), debug_name(name), resident(false), device(device) {}

        // Allow moving but not copying
        BaseResource(const BaseResource&) = delete;
        BaseResource& operator=(const BaseResource&) = delete;
        BaseResource(BaseResource&&) = default;
        BaseResource& operator=(BaseResource&&) = default;

    public:
        virtual ~BaseResource() {}

        virtual void create() = 0;

        virtual void release() = 0;

        std::uint64_t getUid() const { return uid; }
        const std::string& getName() const { return debug_name; }
        bool isResident() const { return resident; }
        VkDeviceSize getSize() const { return size; }  // for now
    };
}
