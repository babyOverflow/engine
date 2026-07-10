#pragma once

#include "Common.h"
#include "MaterialAssetFormat.h"
#include "MeshAssetFormat.h"
#include "ShaderAssetFormat.h"
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

struct MaterialResult {
    MaterialAssetFormat materialAsset;
    AssetPath assetPath;
};

struct MeshResult {
    MeshAssetFormat meshAsset;
    AssetPath assetPath;
};

struct ShaderImportResult {
    ShaderAssetFormat shaderAsset;
    AssetPath assetPath;
};
}  // namespace core::importer
