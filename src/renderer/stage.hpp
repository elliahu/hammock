#pragma once
#include <vector>
#include <string>

#include "stage_types.hpp"
#include "vertex.hpp"
#include "cgltf/cgltf.h"
#include "core/device.hpp"
#include "stage_types.hpp"
#include "vertex.hpp"

namespace hammock::renderer {

    /// @class StageIface
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

    /// @interface StageLoaderIface
    /// Interface for loading stage object from files
    class StageLoaderIface {
       public:
        virtual ~StageLoaderIface() = default;
        virtual void load(const std::string& path, Stage& stage) = 0;
    };


    /// @class GltfLoader
    /// Concrete loader for glTF files
    class GltfLoader : public StageLoaderIface {
       private:
       core::Device& device_;
        // Loaded data
        cgltf_data* data_ = nullptr;

        /// Checks the result and throws if not success
        void checkResult(cgltf_result& result);

        /// Read the file
        void read(const std::string& glTF);

        const uint8_t* getAccessorData(const cgltf_accessor* accessor);

        void loadIndices(const cgltf_accessor* accessor, uint32_t vertexOffset,
            std::vector<uint32_t>& indexBuffer, uint32_t& outFirstIndex, uint32_t& outIndexCount);

        void loadVertices(const cgltf_primitive& prim, std::vector<Vertex>& vertexBuffer,
            uint32_t& outVertexOffset, uint32_t& outVertexCount);

        void visitNode(cgltf_node* gltfNode, Stage& stage, Scene& scene, std::int32_t parentIdx);

        /// Parse the loaded content and fill the stage
        void parse(Stage& stage);

       public:
        GltfLoader(core::Device& device);
        ~GltfLoader() override;

        void load(const std::string& glTF, Stage& stage) override;
    };
}  // namespace hammock::renderer
