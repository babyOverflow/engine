#pragma once
#include "AssetManager.h"
#include "Texture.h"
#include "render.h"
#include "import/Importer.h"

namespace core::render {
class TextureManager {
  public:
    TextureManager(Device* device, AssetManager* assetRepo);

    Handle LoadTexture(const core::importer::TextureResult& assetResult);

    AssetView<Texture> GetTexture(Handle textureHandle) {
        return m_assetRepo->GetTexture(textureHandle);
    }
    AssetView<Texture> GetTexture(const AssetPath& assetPath);

  private:
    static inline TextureAssetFormat kDefaultTextureAsset{
        .width = 1,
        .height = 1,
        .depth = 1,
        .mips = 1,
        .channel = 4,
        .format = TextureFormat::RGBA8Unorm,
        .dimension = TextureDimension::e2D,
        .pixelData = {255, 255, 255, 255},
    };
    Device* m_device;
    AssetManager* m_assetRepo;

    std::unordered_map<AssetPath, Handle> m_textureCache;

};
}  // namespace core::render