#include "gltf_loader.hpp"

void hammock::renderer::GltfLoader::checkResult(cgltf_result& result) {
    if (result != cgltf_result_success) {
        // Error occurred during parsing
        switch (result) {
            case cgltf_result_data_too_short:
                throw std::runtime_error("cgltf error: data too short");
            case cgltf_result_unknown_format:
                throw std::runtime_error("cgltf error: unkown format");
            case cgltf_result_invalid_json:
                throw std::runtime_error("cgltf error: invalid json");
            case cgltf_result_invalid_gltf:
                throw std::runtime_error("cgltf error: invalid glTF");
            case cgltf_result_invalid_options:
                throw std::runtime_error("cgltf error: invalid options");
            case cgltf_result_file_not_found:
                throw std::runtime_error("cgltf error: file not found");
            case cgltf_result_io_error:
                throw std::runtime_error("cgltf error: io error");
            case cgltf_result_out_of_memory:
                throw std::runtime_error("cgltf error: out of memory");
            case cgltf_result_legacy_gltf:
                throw std::runtime_error("cgltf error: legacy glTF");
            default:
                throw std::runtime_error("cgltf error: unknown error");
        }
    }
}
hammock::renderer::GltfLoader::~GltfLoader() { cgltf_free(data_); }
