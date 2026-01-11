module;

#include <cstdint>
#include <vector>

export module hammock_renderer:scene;

import :vertex;
import :math;
import hammock_core;

using namespace hammock::math;

namespace hammock::renderer {

    export struct MeshInstance {
        enum VisibilityFlags : std::int32_t {
            VISIBILITY_NONE = 0,
            VISIBILITY_VISIBLE = 1 << 0,
            VISIBILITY_OPAQUE = 1 << 1,
            VISIBILITY_BLEND = 1 << 2,
            VISIBILITY_CASTS_SHADOW = 1 << 3,
            VISIBILITY_RECEIVES_SHADOW = 1 << 4,
        };

        typedef std::int32_t Index;

        Mat4 transform;
        std::int32_t visibilityFlags;
        Vec3 baseColorFactor;
        Vec3 metallicRoughnessAlphaCutOffFactor;
        Index baseColorTextureIndex;
        Index normalTextureIndex;
        Index metallicRoughnessTextureIndex;
        Index occlusionTextureIndex;
        std::uint32_t firstIndex;
        std::uint32_t indexCount;
    };

    export struct Scene {
        std::vector<MeshInstance> renderMeshes;
        std::vector<Vertex> vertices;
        std::vector<std::uint32_t> indices;
        std::vector<core::ResourceHandle> textures;
    };
}
