#pragma once
#include <cassert>
#include <memory>
#include <utility>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace hammock::core {

    /// @struct Handle
    /// @brief Generational type safe handle
    template <typename T>
    struct Handle {
        uint32_t index = 0;
        uint32_t generation = 0;

        bool operator==(const Handle&) const = default;
    };

    /// @struct ResourceSlot
    /// @brief Resource slot
    template <typename T>
    struct ResourceSlot {
        std::unique_ptr<T> resource;
        uint32_t generation = 1;
    };

    // Forward declaration for use in ResourceRef
    template <typename T>
    class ResourceManager;

    /// @class ResourceRef
    /// @brief Light-weight resource reference that only knows how to get a resource
    template <typename T>
    class ResourceRef {
       public:
        ResourceRef() = default;

        ResourceRef(ResourceManager<T>* manager, Handle<T> handle) : manager(manager), handle(handle) {}

        /// @brief Get the actual resource reference
        T& get() const {
            assert(manager->isValid(handle));
            return manager->get(handle);
        }

        T* operator->() const { return &get(); }

        T& operator*() const { return get(); }

        explicit operator bool() const { return valid(); }

        bool valid() const;

       private:
        ResourceManager<T>* manager = nullptr;
        Handle<T> handle;
    };

    template <typename T>
    bool ResourceRef<T>::valid() const {
        return manager && manager->isValid(handle);
    }

    /// @class ResourceManager
    /// @brief Class for managing resources of a single type
    template <typename T>
    class ResourceManager {
       public:
        /// Destructor releases all resources
        ~ResourceManager() { destroyAll(); }

        /// @brief Create a new resource and return its handle
        template <typename... Args>
        Handle<T> create(Args&&... args) {
            uint32_t index;

            if (!freeList.empty()) {
                index = freeList.back();
                freeList.pop_back();
            } else {
                index = slots.size();
                slots.emplace_back();
            }

            auto& slot = slots[index];

            // Create the resource in-place and store it in unique_ptr
            slot.resource = std::make_unique<T>(std::forward<Args>(args)...);

            return Handle<T>{index, slot.generation};
        }

        /// @brief Destroy a resource by its handle
        void destroy(Handle<T> handle) {
            if (!isValid(handle)) return;

            auto& slot = slots[handle.index];

            slot.resource.reset();  // destroy resource
            slot.generation++;

            freeList.push_back(handle.index);
        }

        /// @brief Destroy all resources
        void destroyAll() {
            for (int i = 0; i < slots.size(); i++) {
                auto& slot = slots[i];
                if (slot.resource) {        // Only destroy if the resource is valid
                    slot.resource.reset();  // destroy resource
                    slot.generation++;
                    freeList.push_back(i);
                }
            }
        }

        /// @brief Destroy all resources and reset to empty state
        void clear() {
            slots.clear();    // unique_ptr destructors fire here
            freeList.clear(); // slots is now empty, freeList must match
        }

        /// @brief Get a resource by its handle
        T& get(Handle<T> handle) {
            auto& slot = slots[handle.index];
            assert(isValid(handle));
            return *slot.resource;
        }

        /// @brief Get a resource by its handle (const)
        const T& get(Handle<T> handle) const {
            const auto& slot = slots[handle.index];
            assert(isValid(handle));
            return *slot.resource;
        }

        /// @brief Check if a handle is valid
        bool isValid(Handle<T> handle) const {
            if (handle.index >= slots.size()) return false;
            const auto& slot = slots[handle.index];
            return slot.resource != nullptr && slot.generation == handle.generation;
        }

        /// @brief Get a resource reference by its handle
        ResourceRef<T> ref(Handle<T> handle) { return ResourceRef<T>(this, handle); }

       private:
        std::vector<ResourceSlot<T>> slots;
        std::vector<uint32_t> freeList;
    };

}  // namespace hammock::core
