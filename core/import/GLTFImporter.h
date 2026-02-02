#pragma once
#include <tiny_gltf.h>

#include "Common.h"
#include "Importer.h"
#include "MaterialAssetFormat.h"
#include "ResourcePool.h"
#include "TextureAssetFormat.h"
#include "render/Mesh.h"

namespace core::importer {
class GLTFImporter {
  public:
    //static std::expected<core::render::Model, Error> ImportFromFile(AssetManager* assetManager,
    //                                                                const render::Device* device,
    //                                                                const std::string& filePath);
    static std::expected<TextureAssetFormat, Error> ImportTextureFromTinygltf(
        const tinygltf::Model& model,
        const tinygltf::Image& image);
};
}  // namespace core::importer