module;

#include <string>
#include <vector>
#include <array>
#include <cstring>
#include <cstdint>

export module hammock.renderer.task_graph:push_constants;

import hammock.renderer.math;

namespace hammock::renderer {

    enum class PushConstantFieldType {
        Undefined, Float32,Int32,Uint32
    };

    // TODO add support for vectors and matrices
    struct PushConstantField {
        std::string name;
        size_t size = 0;
        size_t paddedSize = 0;
        PushConstantFieldType type = PushConstantFieldType::Undefined;
        alignas(4) std::array<std::byte, 32> storage{};
        const void* value = nullptr;

        void setFloat(const float v) {
            static_assert(sizeof(float) == 4);

            std::memset(storage.data(), 0, storage.size());
            std::memcpy(storage.data(), &v, sizeof(float));

            value = storage.data();
            size  = sizeof(float);
            paddedSize = sizeof(float);
            type  = PushConstantFieldType::Float32;
        }

        void setInt(const int32_t v) {
            static_assert(sizeof(int32_t) == 4);

            std::memset(storage.data(), 0, storage.size());
            std::memcpy(storage.data(), &v, sizeof(int32_t));

            value = storage.data();
            size  = sizeof(int32_t);
            paddedSize = sizeof(int32_t);
            type  = PushConstantFieldType::Int32;
        }

        void setUint(const uint32_t v) {
            static_assert(sizeof(uint32_t) == 4);

            std::memset(storage.data(), 0, storage.size());
            std::memcpy(storage.data(), &v, sizeof(uint32_t));

            value = storage.data();
            size  = sizeof(uint32_t);
            paddedSize = sizeof(uint32_t);
            type  = PushConstantFieldType::Int32;
        }

    };

    export class PushConstantsBlock {
    public:
        void addField(const std::string& name, PushConstantFieldType type) {
            fields.push_back({
                .name = name,
                .type = type,
            });
        }

    private:
        std::vector<PushConstantField> fields{};
    };
}