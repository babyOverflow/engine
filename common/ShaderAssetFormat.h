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

    template <typename T = uint32_t>
    static constexpr T kInvalidIdx = static_cast<T>(-1);

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
    };

    struct alignas(64) Header {
        uint32_t magicNumber = SHADER_ASSET_MAGIC;           // 4
        uint16_t version = SHADER_ASSET_VERSION;             // 6
        uint16_t parameterCount = 0;                         // 8
        uint16_t bindingCount = 0;                           // 10
        uint16_t entryPointCount = 0;                        // 12
        uint16_t variableCount = 0;                          // reserved
        uint32_t nameTableSize = 0;                          //
        uint32_t shaderSize = 0;                             //
        uint32_t parameterOffset;                            //
        uint32_t bindingOffset;                              //
        uint32_t entryPointOffset;                           //
        uint32_t variableOffset;                             //
        uint32_t nameTableOffset;                            //
        uint32_t shaderOffset;                               //
        uint16_t passNameIndex = kInvalidIdx<uint16_t>;      //
        uint16_t materialNameIndex = kInvalidIdx<uint16_t>;  //
        char _padding[4] = {0};
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
        uint32_t nameIdx = kInvalidIdx<uint32_t>;
        Resource resource = {.buffer = {0}};
        ResourceType resourceType;
        ShaderVisibility visibility;
        char _padding[10] = {0};

        bool operator==(const Binding& rhs) const {
            return (*this <=> rhs) == std::strong_ordering::equal;
        }

        std::strong_ordering operator<=>(const Binding& rhs) const {
            if (auto cmp = set <=> rhs.set; cmp != 0) {
                return cmp;
            }
            if (auto cmp = binding <=> rhs.binding; cmp != 0) {
                return cmp;
            }
            if (auto cmp = id <=> rhs.id; cmp != 0) {
                return cmp;
            }
            if (auto cmp = nameIdx <=> rhs.nameIdx; cmp != 0) {
                return cmp;
            }
            if (auto cmp = resourceType <=> rhs.resourceType; cmp != 0) {
                return cmp;
            }
            if (auto cmp = visibility <=> rhs.visibility; cmp != 0) {
                return cmp;
            }

            switch (resourceType) {
                case ResourceType::UniformBuffer:
                case ResourceType::StorageBuffer:
                case ResourceType::ReadOnlyStorage:
                    return resource.buffer.bufferSize <=> rhs.resource.buffer.bufferSize;
                case ResourceType::Texture:
                    if (auto cmp = resource.texture.type <=> rhs.resource.texture.type; cmp != 0) {
                        return cmp;
                    }
                    if (auto cmp = resource.texture.viewDimension <=> rhs.resource.texture.viewDimension; cmp != 0) {
                        return cmp;
                    }
                    return resource.texture.multiSampled <=> rhs.resource.texture.multiSampled;
                case ResourceType::Sampler:
                    return resource.sampler.type <=> rhs.resource.sampler.type;
                default:
                    return std::strong_ordering::equal;
            }
        }
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
        uint32_t nameIdx = kInvalidIdx<uint32_t>;

        std::strong_ordering operator<=>(const Variable& rhs) const {
            if (auto cmp = kind <=> rhs.kind; cmp != 0) {
                return cmp;
            }
            if (auto cmp = scalarType <=> rhs.scalarType; cmp != 0) {
                return cmp;
            }
            if (auto cmp = nameIdx <=> rhs.nameIdx; cmp != 0) {
                return cmp;
            }

            switch (kind) {
                case Kind::Scalar:
                    return std::strong_ordering::equal;
                case Kind::Vector:
                    return shape.vector.length <=> rhs.shape.vector.length;
                case Kind::Matrix:
                    if (auto cmp = shape.matrix.rows <=> rhs.shape.matrix.rows; cmp != 0) {
                        return cmp;
                    }
                    return shape.matrix.columns <=> rhs.shape.matrix.columns;
                default:
                    return std::strong_ordering::equal;
            }
        }

        bool operator==(const Variable& rhs) const {
            return (*this <=> rhs) == std::strong_ordering::equal;
        }
    };

    struct ShaderParameter {
        Variable variable;
        uint32_t location;
        Semantic semantic;

        std::strong_ordering operator<=>(const ShaderParameter&) const = default;
    };

    struct EntryPoint {
        ShaderVisibility stage;
        uint32_t nameIdx;
        PropertyId id;

        uint32_t ioStartIndex;
        uint16_t ioCount;

        uint16_t _padding0 = 0;
        uint32_t _padding1 = 0;
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
