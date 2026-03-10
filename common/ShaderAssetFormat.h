#pragma once
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
        uint32_t magicNumber = SHADER_ASSET_MAGIC;
        uint16_t version = SHADER_ASSET_VERSION;
        uint16_t vertexInputCount;
        uint16_t vertexOutputCount;
        uint16_t fragmentOutputCount;
        uint16_t bindingCount;
        uint32_t shaderSize;
        uint32_t threadGroupSize[3];
        ShaderVisibility entryShaderStage;
        char _padding[33];
    };

    /**
     * @brief Flattened Resource's binding information
     *
     */
    struct alignas(32) Binding {
        uint32_t set;
        uint32_t binding;
        uint32_t id;
        uint32_t nameIdx;
        Resource resource = {.buffer = {0}};
        ResourceType resourceType;
        ShaderVisibility visibility;
        char _padding[10];
    };

    struct Variable {
        Kind kind;
        ScalarType scalarType;
        uint8_t rows;
        uint8_t columns;
        uint32_t nameIdx;
    };

    struct ShaderParameter {
        Variable variable;
        uint32_t location;
        uint32_t semanticNameIdx;
    };

    Header header;
    std::vector<ShaderParameter> vertexInputs;
    std::vector<ShaderParameter> vertexOutputs;
    std::vector<ShaderParameter> fragmentOutputs;
    std::vector<Binding> bindings;
    std::vector<uint8_t> code;
    std::vector<std::string> nameTable;

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