module;

#include <cstdint>

export module hammock.renderer.geometry;

import hammock.renderer.vertex;
import hammock.renderer.math;
import hammock.core.base_resource;


namespace hammock::renderer {
    export struct Geometry {
        enum VisibilityFlags : std::int32_t {
            VISIBILITY_NONE = 0,
            VISIBILITY_VISIBLE = 1 << 0,
            VISIBILITY_OPAQUE = 1 << 1,
            VISIBILITY_BLEND = 1 << 2,
            VISIBILITY_CASTS_SHADOW = 1 << 3,
            VISIBILITY_RECEIVES_SHADOW = 1 << 4,
        };

        struct MeshInstance {
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

        std::vector<MeshInstance> renderMeshes;
        std::vector<Vertex> vertices;
        std::vector<std::uint32_t> indices;
        std::vector<ResourceHandle> textures;
    };
}
