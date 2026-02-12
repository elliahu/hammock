#pragma once
#include <string>
#include <vector>
#include <array>
#include <cstring>
#include <cstdint>
#include <memory>

#include "utils/math.hpp"

namespace hammock::renderer {
    /// @enum PushConstantFieldType
    /// @brief Describes the type of the field
    enum class PushConstantFieldType {
        Undefined, Float, Int, Uint
    };


    // TODO add support for vectors and matrices
    /// @class PushConstantField
    /// @brief Represent single field in a push constant block
    class PushConstantField {
        std::string name;
        size_t size = 0;
        size_t paddedSize = 0;
        PushConstantFieldType type = PushConstantFieldType::Undefined;
        alignas(4) std::array<std::byte, 32> storage{};
        const void *value = nullptr;

    public:
        PushConstantField(std::string name, PushConstantFieldType type) : name(std::move(name)), type(type) {
        }

        void setFloat(const float v) {
            static_assert(sizeof(float) == 4);

            std::memset(storage.data(), 0, storage.size());
            std::memcpy(storage.data(), &v, sizeof(float));

            value = storage.data();
            size = sizeof(float);
            paddedSize = sizeof(float);
            type = PushConstantFieldType::Float;
        }

        void setInt(const int32_t v) {
            static_assert(sizeof(int32_t) == 4);

            std::memset(storage.data(), 0, storage.size());
            std::memcpy(storage.data(), &v, sizeof(int32_t));

            value = storage.data();
            size = sizeof(int32_t);
            paddedSize = sizeof(int32_t);
            type = PushConstantFieldType::Int;
        }

        void setUint(const uint32_t v) {
            static_assert(sizeof(uint32_t) == 4);

            std::memset(storage.data(), 0, storage.size());
            std::memcpy(storage.data(), &v, sizeof(uint32_t));

            value = storage.data();
            size = sizeof(uint32_t);
            paddedSize = sizeof(uint32_t);
            type = PushConstantFieldType::Uint;
        }
    };

    /// @class PushConstantsBlock
    /// @brief Represents single push constant block inside a SPIR-V shader
    class PushConstantsBlock {
        std::string name;
        std::vector<std::unique_ptr<PushConstantField>> fields{};
    public:
        explicit PushConstantsBlock(std::string name) : name(std::move(name)) {}

        /// @brief Add field to the block
        /// @returns Reference to the added filed
        std::unique_ptr<PushConstantField> &addField(std::unique_ptr<PushConstantField> &&field) {
            fields.push_back(std::move(field));
            return fields.back();
        }

    };
}
