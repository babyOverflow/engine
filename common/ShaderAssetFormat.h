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
    static constexpr uint32_t SHADER_ASSET_VERSION = 3;

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

    inline void PrintTo(ResourceType type, std::ostream* os) { *os << magic_enum::enum_name(type); }

    enum class ShaderVisibility : uint8_t {
        None = 0x0,
        Vertex = 0x1,
        Fragment = 0x2,
        Compute = 0x4,
        All = Vertex | Fragment | Compute
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

    union Resource {
        Sampler sampler;
        Texture texture;
        Buffer buffer;

        static Resource Buffer(uint32_t size);
    };

    struct alignas(64) Header {
        uint32_t magicNumber = SHADER_ASSET_MAGIC;
        uint16_t version = SHADER_ASSET_VERSION;
        uint16_t bindingCount;
        uint32_t shaderSize;
        uint32_t threadGroupSize[3];
        ShaderVisibility entryShaderStage;
    };

    /**
     * @brief Flattened Resource's binding information
     *
     */
    struct alignas(64) Binding {
        uint32_t set;
        uint32_t binding;
        uint32_t id;
        ResourceType resourceType;
        ShaderVisibility visibility;
        Resource resource = {.buffer = {0}};
        char name[32];
    };

    Header header;
    std::vector<Binding> bindings;
    std::vector<uint8_t> code;

    static std::expected<ShaderAssetFormat, Error> LoadFromMemory(std::span<const uint8_t> memory);
};

inline ShaderAssetFormat::ShaderVisibility operator|(ShaderAssetFormat::ShaderVisibility a,
                                               ShaderAssetFormat::ShaderVisibility b) {
    return static_cast<ShaderAssetFormat::ShaderVisibility>(static_cast<uint8_t>(a) |
                                                      static_cast<uint8_t>(b));
}

}  // namespace core

template <>
struct std::formatter<core::ShaderAssetFormat::ResourceType> : std::formatter<std::string_view> {
    auto format(core::ShaderAssetFormat::ResourceType type, std::format_context& ctx) const {
        std::string_view name = magic_enum::enum_name(type);
        return std::formatter<std::string_view>::format(name, ctx);
    }
};

inline std::ostream& operator<<(std::ostream& os, core::ShaderAssetFormat::ResourceType type) {
    return os << std::format("{}", type);
}

inline uint8_t operator&(core::ShaderAssetFormat::ShaderVisibility a,
                    core::ShaderAssetFormat::ShaderVisibility b) {
    return (static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}