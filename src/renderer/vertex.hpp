#pragma once
#include <compare>
#include <vector>
#include <vulkan/vulkan.hpp>

#include "math.hpp"

using namespace hammock::math;

namespace hammock::renderer {
    struct Vertex {
        Vec3 position{};
        Vec3 normal{};
        Vec2 uv{};
        Vec4 tangent{};

        static std::vector<vk::VertexInputAttributeDescription> getInputAttributeDescriptions() {
            return {{0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, position)},
                {1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal)},
                {2, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, uv)},
                {3, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, tangent)}};
        }

        static std::vector<vk::VertexInputBindingDescription> getInputBindingDescriptions() {
            return {vk::VertexInputBindingDescription{
                0u,
                static_cast<uint32_t>(sizeof(Vertex)),
                vk::VertexInputRate::eVertex,
            }};
        }
    };

    struct UiVertex {
        Vec2 position{};  // screen position
        Vec2 uv{};        // texture coords (0,0 for solid color quads)
        Vec4 color{};     // color
        float useTexture{0.f}; // 0.0 = solid color, 1.0 = sample texture (for text)

        static std::vector<vk::VertexInputAttributeDescription> getInputAttributeDescriptions() {
            return {
                {0, 0, vk::Format::eR32G32Sfloat, offsetof(UiVertex, position)},
                {1, 0, vk::Format::eR32G32Sfloat, offsetof(UiVertex, uv)},
                {2, 0, vk::Format::eR32G32B32A32Sfloat , offsetof(UiVertex, color)},
                {3, 0, vk::Format::eR32Sfloat, offsetof(UiVertex, useTexture)},
            };
        }

        static std::vector<vk::VertexInputBindingDescription> getInputBindingDescriptions() {
            return {vk::VertexInputBindingDescription{
                0u,
                static_cast<uint32_t>(sizeof(UiVertex)),
                vk::VertexInputRate::eVertex,
            }};
        }
    };
}  // namespace hammock::renderer
