module;
#include <chrono>
#include <algorithm>
#include <compare>
#include <vulkan/vulkan.hpp>

module hammock_core;


void hammock::core::ResourceManager::initialize(Device &device, vk::DeviceSize memoryBudget) {
    Singleton<ResourceManager>::initialize(device, memoryBudget);
}

void hammock::core::ResourceManager::releaseResource(uint64_t id) {
    auto it = resources_.find(id);
    if (it != resources_.end()) {
        if (it->second->isResident()) {
            totalMemoryUsed_ -= it->second->getSize();
            it->second->release();
        }
        resources_.erase(it);
        resourceCache_.erase(id);
    }
}

uint64_t hammock::core::ResourceManager::getCurrentTimestamp() {
    // Get the current time point
    auto now = std::chrono::system_clock::now();
    // Convert to duration since epoch in milliseconds
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    // Return the count of milliseconds as uint64_t
    return static_cast<uint64_t>(duration.count());
}

void hammock::core::ResourceManager::evictResources(vk::DeviceSize requiredSize) {
    // Sort resources by last used time and use count
    std::vector<std::pair<uint64_t, CacheEntry> > sortedCache;
    for (const auto &entry: resourceCache_) {
        sortedCache.push_back(entry);
    }

    std::sort(sortedCache.begin(), sortedCache.end(),
              [](const auto &a, const auto &b) {
                  // Consider both last used time and use frequency
                  return (a.second.lastUsed * a.second.useCount) <
                         (b.second.lastUsed * b.second.useCount);
              });

    // Unload resources until we have enough space
    vk::DeviceSize freedMemory = 0;
    for (const auto &entry: sortedCache) {
        auto *resource = resources_[entry.first].get();
        if (resource->isResident()) {
            resource->release();
            freedMemory += resource->getSize();
            totalMemoryUsed_ -= resource->getSize();

            if (freedMemory >= requiredSize) {
                break;
            }
        }
    }
}
