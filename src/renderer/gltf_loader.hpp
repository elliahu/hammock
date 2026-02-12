#pragma once
#include <stdexcept>
#include <string>

#include "cgltf/cgltf.h"
#include "gltf_loader.hpp"

/// hammock uses glTF 2 file format for stage representation
/// and cgltf library for glTF parsing and loading

namespace hammock::renderer {
    class GltfLoader {
       private:
        // Loaded data
        cgltf_data* data_ = nullptr;

        /// Checks the result and throws if not success
        void checkResult(cgltf_result& result);

       public:
        ~GltfLoader();

        void load(const std::string& glTF) {
            // Loading library options
            cgltf_options options = {
                .type = cgltf_file_type::cgltf_file_type_invalid,  // autodetect
                .json_token_count = 0,                             // autodetect
            };

            // Parse file
            cgltf_result result = cgltf_parse_file(&options, glTF.c_str(), &data_);

            // Check result
            try {
                checkResult(result);
            } catch (std::runtime_error err) {
                throw std::runtime_error("failed to parse glTF file: " + std::string(err.what()));
            }
        }
    };
}  // namespace hammock::renderer