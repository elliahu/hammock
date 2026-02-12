#pragma once
#include <stdexcept>
#include <string>

#include "cgltf/cgltf.h"
#include "core/utilities.hpp"
#include "gltf_loader.hpp"
#include "math.hpp"
#include "stage.hpp"
#include "stage_types.hpp"
#include "utils/math.hpp"

/// hammock uses glTF 2 file format for stage representation
/// and cgltf library for glTF parsing and loading

namespace hammock::renderer {

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
        // Loaded data
        cgltf_data* data_ = nullptr;

        /// Checks the result and throws if not success
        void checkResult(cgltf_result& result);

        /// Read the file
        void read(const std::string& glTF);

        void visitNode(cgltf_node* gltfNode, Stage& stage, Scene& scene, std::int32_t parentIdx) {
            // TODO apply transform
            // TODO draw mesh if present

            // Create the node
            SceneNode node{};

            // Camera
            if (gltfNode->camera) {
                node.baseType = SceneNodeBaseType::Camera;
                cgltf_camera* gltfCamera = gltfNode->camera;
                Camera camera{};

                // Decide camera type
                if (gltfCamera->type == cgltf_camera_type_perspective) {
                    // Perspective camera
                    camera.type = CameraType::Perspective;
                    if (!gltfCamera->data.perspective.has_aspect_ratio) {
                        throw std::runtime_error("camera does not have aspect ratio set");
                    }

                    if (!gltfCamera->data.perspective.has_zfar) {
                        throw std::runtime_error("camera does not have zfar");
                    }
                    camera.perspective.aspectRatio = gltfCamera->data.perspective.aspect_ratio;
                    camera.perspective.yfov = gltfCamera->data.perspective.yfov;
                    camera.perspective.znear = gltfCamera->data.perspective.znear;
                    camera.perspective.zfar = gltfCamera->data.perspective.zfar;
                } else if (gltfCamera->type == cgltf_camera_type_orthographic) {
                    // Ortho camera
                    camera.type = CameraType::Orthographic;
                    camera.orthographic.xmag = gltfCamera->data.orthographic.xmag;
                    camera.orthographic.ymag = gltfCamera->data.orthographic.ymag;
                    camera.orthographic.zfar = gltfCamera->data.orthographic.zfar;
                    camera.orthographic.znear = gltfCamera->data.orthographic.znear;
                } else {
                    throw std::runtime_error("invalid camera type");
                }
                // Set the node
                node.camera = camera;
            } else if (gltfNode->light) {
                // Node is a light
                node.baseType = SceneNodeBaseType::Light;
                cgltf_light* gltfLight = gltfNode->light;
                Light light{};

                // Decide light type
                if (gltfLight->type == cgltf_light_type_point) {
                    light.type = LightType::Punctual;
                    light.punctual.color = {gltfLight->color[0], gltfLight->color[1], gltfLight->color[2]};
                    light.punctual.intensity = gltfLight->intensity;
                } else {
                    core::Logger::warn("Skiped a light during loading: unsupported type");
                }

                // Set the node
                node.light = light;
            } else {
                // Node is a mesh
                if (!gltfNode->mesh) {
                    throw std::runtime_error("gltf node expected to be mesh is not a mesh");
                }

                
            }

            // Add the node to the stage
            stage.nodes.push_back(node);
            std::int32_t nodeIdx = stage.nodes.size() - 1;

            // Add the node to the scene
            scene.nodes.push_back(nodeIdx);
            scene.hierarchies.push_back(Hierarchy{.parent = parentIdx});

            // Get the local transform and compute the world mat4
            // Can be transform (translation, rotation, scale) or matrix or nothing at all
            Transform transform{};
            math::Mat4 world = math::identity();
            if (gltfNode->has_matrix) {
                core::Logger::warn(
                    "node transform described by mat4, which is currently unsupported. skipping.");
            } else if (gltfNode->has_translation) {
                transform.position = {
                    gltfNode->translation[0], gltfNode->translation[1], gltfNode->translation[2]};
            } else if (gltfNode->has_rotation) {
                transform.rotation = {gltfNode->rotation[0],
                    gltfNode->rotation[1],
                    gltfNode->rotation[2],
                    gltfNode->rotation[3]};
            } else if (gltfNode->has_scale) {
                transform.scale = {gltfNode->scale[0], gltfNode->scale[1], gltfNode->scale[2]};
            }
            scene.locals.push_back(transform);
            scene.worlds.push_back(world);

            // Recursively visit children
            for (cgltf_size i = 0; i < gltfNode->children_count; ++i) {
                visitNode(gltfNode->children[i], stage, scene, nodeIdx);
            }
        }

        /// Parse the loaded content and fill the stage
        void parse(Stage& stage) {
            // First of all start with the scenes
            for (cgltf_size i = 0; i < data_->scenes_count; i++) {
                cgltf_scene& gltfScene = data_->scenes[i];
                stage.scenes.push_back(Scene{});

                // Visit root nodes
                for (cgltf_size j = 0; j < gltfScene.nodes_count; j++) {
                    visitNode(gltfScene.nodes[j], stage, stage.scenes[i], -1);
                }
            }
        }

       public:
        ~GltfLoader() override;

        void load(const std::string& glTF, Stage& stage) override;
    };
}  // namespace hammock::renderer