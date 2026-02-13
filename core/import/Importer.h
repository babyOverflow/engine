#pragma once
#include <expected>

#include "Common.h"
#include "TextureAssetFormat.h"
namespace core::render {
class Device;
}

namespace core::render {
class LayoutCache;

}  // namespace core::render

namespace core {
class AssetManager;
}

namespace core::importer {
struct TextureResult {
    TextureAssetFormat textureAsset;
    AssetPath assetPath;
};
}  // namespace core::importer