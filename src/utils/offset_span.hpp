#pragma once
#include <cstdint>
#include <cassert>

namespace hammock{

    /// @brief A relocatable pointer-less array view based on a byte offset
    template<typename T>
    struct OffsetSpan {
        std::uint32_t offsetBytes; // Offset from the START of the RenderSnapshot buffer
        std::uint32_t elementCount;

        // Resolves the offset to a real pointer given the base address of the snapshot
        const T* data(const void* snapshotBase) const {
            if (elementCount == 0) return nullptr;
            return reinterpret_cast<const T*>(static_cast<const std::uint8_t*>(snapshotBase) + offsetBytes);
        }

        T* data(void* snapshotBase) {
            if (elementCount == 0) return nullptr;
            return reinterpret_cast<T*>(static_cast<std::uint8_t*>(snapshotBase) + offsetBytes);
        }

        // Helper to get an element safely
        const T& operator[](std::size_t index) const {
            // Note: Requires passing base or keeping it locally.
            // Often, backends loop via: const T* arr = span.data(snapshot);
            assert(index < elementCount);
        }
    };
}
