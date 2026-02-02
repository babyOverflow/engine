#pragma once
#include <vector>

namespace core {

enum class TextureFormat : uint8_t {
    RGBA8Unorm,
    Unknown,
};

enum class TextureDimension : uint8_t { e2D, Unknown };

struct TextureAssetFormat {
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t depth = 1;
    uint32_t mips = 1;
    uint32_t channel;

    TextureFormat format = TextureFormat::RGBA8Unorm;
    TextureDimension dimension = TextureDimension::e2D;

    std::vector<uint8_t> pixelData;
};
}  // namespace core