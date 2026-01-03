module;
#include <memory>
#include <unordered_map>
#include <compare>
#include <vulkan/vulkan.hpp>

export module hammock.core.resource_manager;

import hammock.core.utilities;
import hammock.core.buffer;
import hammock.core.image;
import hammock.core.device;
import hammock.core.base_resource;


namespace hammock::core {
    export class ResourceManager;

    /// @class ResourceFactory
    /// At this point it is not really necessary :)
    class ResourceFactory final {
        friend class ResourceManager;

        template <typename T, typename... Args>
        static std::unique_ptr<T> create(Device& device, uint64_t id, Args&&... args) {
            return std::make_unique<T>(device, id, std::forward<Args>(args)...);
        }
    };

    /// @class ResourceManager
    /// Instance of this class is responsible for keeping and cleaning resources allocated on the GPU
    class ResourceManager final : public Singleton<ResourceManager> {
        friend class Singleton<ResourceManager>;

       private:
        using ResourceMap = std::unordered_map<uint64_t, std::unique_ptr<BaseResource> >;

        Device& device_;
        ResourceMap resources_;

        vk::DeviceSize totalMemoryUsed_;
        vk::DeviceSize memoryBudget_;
        uint64_t nextId_;

        // Cache for frequently used resources
        struct CacheEntry {
            uint64_t lastUsed;
            uint64_t useCount;
        };

        std::unordered_map<uint64_t, CacheEntry> resourceCache_;

        explicit ResourceManager(Device& device, vk::DeviceSize memoryBudget = 6ULL * 1024 * 1024 * 1024)
            // 6GB default
            : device_(device), totalMemoryUsed_(0), memoryBudget_(memoryBudget), nextId_(1) {}

       public:
        /// @brief Initializes the singleton instance
        static void initialize(Device& device, vk::DeviceSize memoryBudget = 6ULL * 1024 * 1024 * 1024);

        /// @brief Create a resource
        template <typename T, typename... Args>
        [[nodiscard]] ResourceHandle createResource(Args&&... args);

        template <typename T, typename... Args>
        ResourceHandle addResource(Args&&... args);

        template <typename T>
        T* getResource(ResourceHandle handle);

        void releaseResource(uint64_t id);

       private:
        static uint64_t getCurrentTimestamp();

        void evictResources(vk::DeviceSize requiredSize);
    };

    template<typename T, typename ... Args>
    ResourceHandle ResourceManager::createResource(Args &&...args) {
        static_assert(ResourceTypeTraits<T>::type != ResourceType::Invalid,
                      "Resource type not registered in ResourceTypeTraits");

        auto resource = ResourceFactory::create<T>(device_, nextId_, std::forward<Args>(args)...);
        uint64_t id = nextId_++;

        // if (totalMemoryUsed + resource->getSize() > memoryBudget) {
        //     evictResources(resource->getSize());
        // }

        resource->create();

        resources_[id] = std::move(resource);
        resourceCache_[id] = {getCurrentTimestamp(), 0};

        return ResourceHandle::create(ResourceTypeTraits<T>::type, id);
    }

    template<typename T, typename ... Args>
    ResourceHandle ResourceManager::addResource(Args &&...args) {
        static_assert(ResourceTypeTraits<T>::type != ResourceType::Invalid,
                      "Resource type not registered in ResourceTypeTraits");

        auto resource = ResourceFactory::create<T>(device_, nextId_, std::forward<Args>(args)...);
        uint64_t id = nextId_++;

        resources_[id] = std::move(resource);
        resourceCache_[id] = {getCurrentTimestamp(), 0};

        return ResourceHandle::create(ResourceTypeTraits<T>::type, id);
    }

    template<typename T>
    T * ResourceManager::getResource(ResourceHandle handle) {
        // Type check
        if (ResourceTypeTraits<T>::type != handle.getType()) {
            return nullptr;
        }

        auto it = resources_.find(handle.getUid());
        if (it != resources_.end()) {
            auto* resource = static_cast<T*>(it->second.get());

            // Update cache information
            resourceCache_[handle.getUid()].lastUsed = getCurrentTimestamp();
            resourceCache_[handle.getUid()].useCount++;

            // Load if not resident
            if (!resource->isResident()) {
                resource->create();
                totalMemoryUsed_ += resource->getSize();
            }
            return resource;
        }
        return nullptr;
    }
}  // namespace hammock::core
