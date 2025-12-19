module;

#include <vulkan/vulkan.h>
#include <vector>

export module hammock.renderer.vertex;

import hammock.renderer.math;

using namespace hammock::math;

namespace hammock::renderer {
    export struct Vertex {
        Vec3 position{};
        Vec3 normal{};
        Vec2 uv{};
        Vec4 tangent{};

        static std::vector<VkVertexInputAttributeDescription> vertexInputAttributeDescriptions() {
            return {
                    {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)},
                    {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)},
                    {2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv)},
                    {3, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, tangent)}
            };
        }

        static std::vector<VkVertexInputBindingDescription> vertexInputBindingDescriptions() {
            return {
                    VkVertexInputBindingDescription{
                        .binding = 0,
                        .stride = static_cast<uint32_t>(sizeof(Vertex)),
                        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
                    }
            };
        }
    };

    export struct Triangle {
        Vertex v0,v1,v2;
    };

}
