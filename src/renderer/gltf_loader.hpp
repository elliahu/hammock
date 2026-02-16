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
#include "vertex.hpp"
#include "vulkan_context.hpp"

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
        core::VulkanContext& ctx_;
        // Loaded data
        cgltf_data* data_ = nullptr;

        /// Checks the result and throws if not success
        void checkResult(cgltf_result& result);

        /// Read the file
        void read(const std::string& glTF);

        const uint8_t* getAccessorData(const cgltf_accessor* accessor) {
            const cgltf_buffer_view* view = accessor->buffer_view;
            const cgltf_buffer* buffer = view->buffer;

            return static_cast<const uint8_t*>(buffer->data) + view->offset + accessor->offset;
        }

        void loadIndices(const cgltf_accessor* accessor, uint32_t vertexOffset,
            std::vector<uint32_t>& indexBuffer, uint32_t& outFirstIndex, uint32_t& outIndexCount) {
            outFirstIndex = indexBuffer.size();
            outIndexCount = accessor->count;

            indexBuffer.resize(outFirstIndex + outIndexCount);

            const uint8_t* src = getAccessorData(accessor);

            switch (accessor->component_type) {
                case cgltf_component_type_r_8u: {
                    const uint8_t* in = (const uint8_t*)src;
                    for (uint32_t i = 0; i < outIndexCount; ++i)
                        indexBuffer[outFirstIndex + i] = vertexOffset + in[i];
                    break;
                }
                case cgltf_component_type_r_16u: {
                    const uint16_t* in = (const uint16_t*)src;
                    for (uint32_t i = 0; i < outIndexCount; ++i)
                        indexBuffer[outFirstIndex + i] = vertexOffset + in[i];
                    break;
                }
                case cgltf_component_type_r_32u: {
                    const uint32_t* in = (const uint32_t*)src;
                    for (uint32_t i = 0; i < outIndexCount; ++i)
                        indexBuffer[outFirstIndex + i] = vertexOffset + in[i];
                    break;
                }
                default:
                    throw std::runtime_error("Unsupported index type");
            }
        }

        void loadVertices(const cgltf_primitive& prim, std::vector<Vertex>& vertexBuffer,
            uint32_t& outVertexOffset, uint32_t& outVertexCount) {
            const cgltf_accessor* posAcc = nullptr;
            const cgltf_accessor* nrmAcc = nullptr;
            const cgltf_accessor* uvAcc = nullptr;
            const cgltf_accessor* tngAcc = nullptr;

            for (cgltf_size i = 0; i < prim.attributes_count; ++i) {
                const cgltf_attribute& attr = prim.attributes[i];
                if (attr.type == cgltf_attribute_type_position)
                    posAcc = attr.data;
                else if (attr.type == cgltf_attribute_type_normal)
                    nrmAcc = attr.data;
                else if (attr.type == cgltf_attribute_type_texcoord)
                    uvAcc = attr.data;
                else if (attr.type == cgltf_attribute_type_tangent)
                    tngAcc = attr.data;
            }

            if (!posAcc) throw std::runtime_error("primitive has no POSITION");
            if (!nrmAcc) throw std::runtime_error("primitive as not NORMAL");
            if (!uvAcc) throw std::runtime_error("primitive as not TEXCOORD_0");
            if (!tngAcc) throw std::runtime_error("primitive as not TANGENT");

            outVertexOffset = vertexBuffer.size();
            outVertexCount = posAcc->count;

            vertexBuffer.resize(outVertexOffset + outVertexCount);

            const float* pos = reinterpret_cast<const float*>(getAccessorData(posAcc));
            const float* nrm = nrmAcc ? reinterpret_cast<const float*>(getAccessorData(nrmAcc)) : nullptr;
            const float* uv = uvAcc ? reinterpret_cast<const float*>(getAccessorData(uvAcc)) : nullptr;
            const float* tng = tngAcc ? reinterpret_cast<const float*>(getAccessorData(tngAcc)) : nullptr;

            for (uint32_t i = 0; i < outVertexCount; ++i) {
                Vertex& v = vertexBuffer[outVertexOffset + i];
                v.position = {
                    pos[i * 3 + 0],
                    pos[i * 3 + 1],
                    pos[i * 3 + 2],
                };
                v.normal = nrm ? math::Vec3{nrm[i * 3 + 0], nrm[i * 3 + 1], nrm[i * 3 + 2]}
                               : math::Vec3{0.f, 1.f, 0.f};
                v.uv = uv ? math::Vec2{uv[i * 2 + 0], uv[i * 2 + 1]} : math::Vec2{0.f, 0.f};
                v.tangent = {tng[i * 4 + 0], tng[i * 4 + 1], tng[i * 4 + 2], tng[i * 4 + 3]};
            }
        }

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
                    light.color = {gltfLight->color[0], gltfLight->color[1], gltfLight->color[2]};
                    light.intensity = gltfLight->intensity;
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
                node.baseType = SceneNodeBaseType::Mesh;
                cgltf_mesh* gltfMesh = gltfNode->mesh;
                Mesh mesh{};
                mesh.firstInstance =
                    stage.primitives.size() == 0 ? stage.primitives.size() : stage.primitives.size() - 1;
                mesh.instanceCount = static_cast<std::uint32_t>(gltfMesh->primitives_count);

                // Loop over the primitives
                for (cgltf_size i = 0; i < gltfMesh->primitives_count; i++) {
                    cgltf_primitive& gltfPrimitive = gltfMesh->primitives[i];
                    Primitive primitive{};

                    // Currently hammock supports only triangle primitive type (no triangle strips, points
                    // etc)
                    if (gltfPrimitive.type != cgltf_primitive_type_triangles) {
                        throw std::runtime_error("non-triangle primitive types not supported");
                    }

                    // Load material
                    cgltf_material* gltfMat = gltfPrimitive.material;
                    // glTF allows null material → default material
                    if (!gltfMat) {
                        Material mat{};
                        mat.doubleSided = false;

                        mat.baseColorFactor = {1.f, 1.f, 1.f, 1.f};
                        mat.metallicFactor = 1.f;
                        mat.roughnessFactor = 1.f;

                        stage.materials.push_back(mat);
                        primitive.materialIndex = stage.materials.size() - 1;
                    }

                    Material mat{};
                    mat.doubleSided = gltfMat->double_sided;

                    const cgltf_pbr_metallic_roughness& pbr = gltfMat->pbr_metallic_roughness;
                    mat.baseColorFactor = {pbr.base_color_factor[0],
                        pbr.base_color_factor[1],
                        pbr.base_color_factor[2],
                        pbr.base_color_factor[3]};

                    // Metallic / roughness
                    mat.metallicFactor = pbr.metallic_factor;
                    mat.roughnessFactor = pbr.roughness_factor;
                    
                    // Place the material
                    stage.materials.push_back(mat);
                    primitive.materialIndex = stage.materials.size() - 1;

                    // Load vertices and indices
                    uint32_t vertexOffset = 0;
                    uint32_t vertexCount = 0;
                    loadVertices(gltfPrimitive, stage.vertexBuffer, vertexOffset, vertexCount);

                    if (gltfPrimitive.indices) {
                        loadIndices(gltfPrimitive.indices,
                            vertexOffset,
                            stage.indexBuffer,
                            primitive.firstIndex,
                            primitive.indexCount);
                    } else {
                        // Generate indices
                        primitive.firstIndex = stage.indexBuffer.size();
                        primitive.indexCount = vertexCount;
                        for (uint32_t i = 0; i < vertexCount; ++i)
                            stage.indexBuffer.push_back(vertexOffset + i);
                    }

                    stage.primitives.push_back(primitive);
                }
                node.mesh = mesh;
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
        GltfLoader(core::VulkanContext& ctx);
        ~GltfLoader() override;

        void load(const std::string& glTF, Stage& stage) override;
    };
}  // namespace hammock::renderer