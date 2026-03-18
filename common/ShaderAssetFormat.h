#pragma once
#include <compare>
#include <expected>
#include <format>
#include <iostream>
#include <magic_enum/magic_enum.hpp>
#include <span>

#include "Common.h"

namespace core {
struct ShaderAssetFormat {
    static constexpr uint32_t SHADER_ASSET_MAGIC = 0x52444853;
    static constexpr uint32_t SHADER_ASSET_VERSION = 4;

    static constexpr uint32_t kInvalidIdx = -1;

    enum class Flag : uint16_t {  // Reserved
        None,
    };

    enum class Kind : uint8_t {
        None,
        Scalar,
        Vector,
        Matrix,
        Struct,
    };

    enum class SamplerType : uint8_t {
        BindingNotUsed,
        Undefined,
        Filtering,
        NonFiltering,
        Comparison,
    };

    enum class ViewDimension : uint8_t {
        Undefined,
        e1D,
        e2D,
        e2DArray,
        Cube,
        CubeArray,
        e3D,
    };

    enum class TextureType : uint8_t {
        BindingNotUsed,
        Undefined,
        Float,
        UnfilterableFloat,
        Depth,
        Sint,
        Uint,
        Bool,
    };

    enum class ResourceType : uint8_t {
        UniformBuffer,
        StorageBuffer,
        ReadOnlyStorage,
        Texture,
        Sampler,
        Unknown = 0xFF,
    };

    enum class ScalarType : uint8_t {
        F32,
        F16,
        U32,
        I32,
        Bool,
        Undefined,
    };

    inline void PrintTo(ResourceType type, std::ostream* os) { *os << magic_enum::enum_name(type); }

    enum class ShaderVisibility : uint8_t {
        None = 0x0,
        Vertex = 0x1,
        Fragment = 0x2,
        Render = 0x03,  // Vertex | Fragment
        Compute = 0x4,
        All = 0x07,  // Vertex | Fragment | Compute
    };

    struct Sampler {
        SamplerType type;
    };

    struct Texture {
        TextureType type = TextureType::Undefined;
        ViewDimension viewDimension = ViewDimension::Undefined;
        uint8_t multiSampled = 0;
    };

    struct Buffer {
        uint32_t bufferSize;
    };

    struct Resource {
        union {
            Sampler sampler;
            Texture texture;
            Buffer buffer;
        };

        static Resource Buffer(uint32_t size);

        bool operator==(const Resource& rhs) const {
            return std::memcmp(this, &rhs, sizeof(Resource)) == 0;
        }
        std::strong_ordering operator<=>(const Resource& rhs) const {
            int res = std::memcmp(this, &rhs, sizeof(Resource));
            return res <=> 0;
        }
    };

    struct alignas(64) Header {
        uint32_t magicNumber = SHADER_ASSET_MAGIC;
        uint16_t version = SHADER_ASSET_VERSION;
        uint16_t parameterCount;
        uint16_t bindingCount;
        uint16_t entryPointCount;
        uint16_t variableCount;  // reserved
        uint16_t indexCount;
        uint32_t nameTableSize;
        uint32_t shaderSize;
        uint32_t parameterOffset;
        uint32_t bindingOffset;
        uint32_t entryPointOffset;
        uint32_t variableOffset;
        uint32_t indexOffset;
        uint32_t nameTableOffset;
        uint32_t shaderOffset;
        char _padding[12] = {0};
    };

    static_assert(sizeof(Header) == 64);

    /**
     * @brief Flattened Resource's binding information
     *
     */
    struct alignas(32) Binding {
        uint32_t set;
        uint32_t binding;
        uint32_t id;
        uint32_t nameIdx = kInvalidIdx;
        Resource resource = {.buffer = {0}};
        ResourceType resourceType;
        ShaderVisibility visibility;
        char _padding[10] = {0};

        std::strong_ordering operator<=>(const Binding&) const = default;
    };

    struct Scalar {};
    struct Vector {
        uint8_t length;
    };
    struct Matrix {
        uint8_t rows;
        uint8_t columns;
    };

    struct Shape {
        union {
            Scalar scalar;
            Vector vector;
            Matrix matrix;
        };

        static Shape Scalar() { return Shape{.scalar = {}}; }
        static Shape Vector(uint8_t count);
        static Shape Matrix(uint8_t rows, uint8_t columns);
    };

    struct Variable {
        Kind kind;
        ScalarType scalarType;
        Shape shape;
        uint32_t nameIdx = kInvalidIdx;

        bool operator==(const Variable& rhs) const {
            if (kind != rhs.kind || scalarType != rhs.scalarType || nameIdx != rhs.nameIdx) {
                return false;
            }

            switch (kind) {
                case Kind::Scalar:
                    return true;
                case Kind::Vector:
                    return shape.vector.length == rhs.shape.vector.length;
                case Kind::Matrix:
                    return shape.matrix.rows == rhs.shape.matrix.rows &&
                           shape.matrix.columns == rhs.shape.matrix.columns;
                default:
                    return true;
            }
        }
    };

    struct ShaderParameter {
        Variable variable;
        uint32_t location;
        uint32_t semanticNameIdx = kInvalidIdx;

        std::strong_ordering operator<=>(const ShaderParameter&) const = default;
    };

    struct EntryPoint {
        ShaderVisibility stage;
        uint32_t nameIdx;

        uint32_t ioStartIndex;
        uint32_t bindingStartIndex;

        uint16_t ioCount;
        uint16_t bindingCount;

        uint32_t _padding;
    };
    static_assert(sizeof(EntryPoint) == 24, "EntryPoint size must be 24 bytes!");

    Header header;
    std::vector<ShaderParameter> parameters;
    std::vector<Binding> bindings;
    std::vector<Variable> variables;  // reserved
    std::vector<EntryPoint> entryPoints;
    std::vector<uint8_t> code;
    std::string nameTableData;
    std::vector<std::string_view> tokens;
    std::vector<uint32_t> indices;

    static std::expected<ShaderAssetFormat, Error> LoadFromMemory(std::span<const uint8_t> memory);
};

}  // namespace core

template <>
struct std::formatter<core::ShaderAssetFormat::ResourceType> : std::formatter<std::string_view> {
    auto format(core::ShaderAssetFormat::ResourceType type, std::format_context& ctx) const {
        std::string_view name = magic_enum::enum_name(type);
        return std::formatter<std::string_view>::format(name, ctx);
    }
};

inline core::ShaderAssetFormat::ShaderVisibility operator|(
    core::ShaderAssetFormat::ShaderVisibility a,
    core::ShaderAssetFormat::ShaderVisibility b) {
    return static_cast<core::ShaderAssetFormat::ShaderVisibility>(static_cast<uint8_t>(a) |
                                                                  static_cast<uint8_t>(b));
}

inline core::ShaderAssetFormat::ShaderVisibility operator&(
    core::ShaderAssetFormat::ShaderVisibility a,
    core::ShaderAssetFormat::ShaderVisibility b) {
    return static_cast<core::ShaderAssetFormat::ShaderVisibility>(static_cast<uint8_t>(a) &
                                                                  static_cast<uint8_t>(b));
}