#pragma once
#include <string>

#include "cgltf/cgltf.h"
#include "core/device.hpp"
#include "stage.hpp"
#include "stage_types.hpp"
#include "vertex.hpp"
#include "stage_loader_iface.hpp"

/// hammock uses glTF 2 file format for stage representation
/// and cgltf library for glTF parsing and loading

namespace hammock::renderer {

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
