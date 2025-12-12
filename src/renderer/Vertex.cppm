export module hammock.renderer.vertex;

import std;
import renderer.math;

#include <vulkan/vulkan.h>;


namespace hammock::renderer {
    export struct Vertex {
        Math::Vec3 position{};
        Math::Vec3 normal{};
        Math::Vec2 uv{};
        Math::Vec4 tangent{};

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
