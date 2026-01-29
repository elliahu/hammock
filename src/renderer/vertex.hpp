#pragma once
#include <vector>
#include <compare>
#include <vulkan/vulkan.hpp>

#include "math.hpp"


using namespace hammock::math;

namespace hammock::renderer {
    struct Vertex {
        Vec3 position{};
        Vec3 normal{};
        Vec2 uv{};
        Vec4 tangent{};

        static std::vector<vk::VertexInputAttributeDescription> vertexInputAttributeDescriptions() {
            return {
                {0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, position)},
                {1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal)},
                {2, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, uv)},
                {3, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, tangent)}
            };
        }

        static std::vector<vk::VertexInputBindingDescription> vertexInputBindingDescriptions() {
            return {
                vk::VertexInputBindingDescription{
                    0u,
                     static_cast<uint32_t>(sizeof(Vertex)),
                     vk::VertexInputRate::eVertex,
                }
            };
        }
    };

    struct Triangle {
        Vertex v0, v1, v2;
    };
}
