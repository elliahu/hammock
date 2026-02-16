#pragma once
#include <array>
#include <cstdint>
#include <vector>

#include "math.hpp"

namespace hammock::renderer {

    /// @enum CameraType
    enum class CameraType : std::uint8_t { Perspective, Orthographic };

    /// @struct Camera
    /// @brief Represents a camera
    struct Camera {
        struct PerspectiveCamera {
            float yfov;  // radians
            float aspectRatio;
            float znear;
            float zfar;
        };

        struct OrthographicCamera {
            float xmag;
            float ymag;
            float zfar;
            float znear;
        };
        CameraType type = CameraType::Perspective;
        std::uint32_t node;  // Index into spacial graph
        union {
            PerspectiveCamera perspective;
            OrthographicCamera orthographic;
        };
    };

    /// @enum LightType
    enum class LightType : std::uint8_t { Punctual };

    /// @struct Light
    /// Represents a light source
    struct Light {
        LightType type;
        std::array<float, 3> color;
        float intensity;
    };

    /// @struct Material
    /// Represents a material
    /// @note For now, we ignore textured materials for simplicity
    struct Material {
        struct PbrMetallicRoughness {};
        struct PbrSpecularGlossiness {};
        bool doubleSided;
        std::array<float, 4> baseColorFactor = {1.f, 1.f, 1.f, 1.f};
        float roughnessFactor = 1.f;
        float metallicFactor = 1.f;
    };

    /// @struct Primitive
    /// Represents a single drawable entity (like a single draw call)
    struct Primitive {
        std::uint32_t materialIndex;
        std::uint32_t firstIndex;
        std::uint32_t indexCount;
    };

    /// @struct Mesh
    /// Represents renderable mesh
    struct Mesh {
        std::uint32_t firstInstance;
        std::uint32_t instanceCount;
    };

    /// @enum SceneNodeBaseType
    /// Base type of a general scene node
    enum class SceneNodeBaseType : std::uint8_t { Camera, Light, Mesh };

    /// @struct SceneNode
    /// Describes the node in the scene which can be of many types
    /// Composition used instead of inheritance for better performance
    struct SceneNode {
        SceneNodeBaseType baseType;
        Camera camera;
        Light light;
        Mesh mesh;
    };

    /// @struct Hierarchy
    /// Used to represent tree like structures
    struct Hierarchy {
        std::int32_t parent{-1};
        // TODO first chid, next sibling
    };

    /// @struct Transform
    /// Represents transformation of a scene node
    struct Transform {
        math::Vec3 position{0.f, 0.f, 0.f};
        math::Quat rotation{1.f, 0.f, 0.f, 0.f};
        math::Vec3 scale{1.f, 1.f, 1.f};
    };

    /// @struct Scene
    /// Scene represents hierarchy of nodes
    struct Scene {
        std::vector<std::uint32_t> nodes;
        std::vector<Hierarchy> hierarchies;
        std::vector<Transform> locals;
        std::vector<math::Mat4> worlds;
    };
}  // namespace hammock::renderer