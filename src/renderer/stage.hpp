#pragma once
#include <vector>

#include "stage_types.hpp"
#include "vertex.hpp"

namespace hammock::renderer {

    class StageIface {
       public:
        virtual ~StageIface() = default;
    };

    /// @class Stage
    /// @brief Stage is the space where all the rendering data live
    /// Stage contains scenes, nodes, materials etc. and is mutable representation of a the whole world
    class Stage : public StageIface {
        friend class GltfLoader;

       private:
        std::vector<Scene> scenes;
        std::vector<SceneNode> nodes;
        std::vector<Material> materials;
        std::vector<Primitive> primitives;
        std::vector<Mesh> meshes;
        std::vector<Camera> cameras;
        std::vector<Light> lights;
        std::vector<Vertex> vertexBuffer{};
        std::vector<std::uint32_t> indexBuffer{};

       public:
        ~Stage() override = default;
    };

}  // namespace hammock::renderer
