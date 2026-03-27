#pragma once
#include <expected>

#include "Common.h"
#include "MaterialAssetFormat.h"
#include "MeshAssetFormat.h"
#include "ModelAssetFormat.h"
#include "TextureAssetFormat.h"
#include "render/ShaderAsset.h"

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
    ShaderAssetFormat shaderBlob;
    AssetPath assetPath;
};
}  // namespace core::importer