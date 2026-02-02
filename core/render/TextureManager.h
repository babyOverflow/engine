#pragma once
#include "AssetManager.h"
#include "Texture.h"
#include "render.h"

namespace core::render {
class TextureManager {
  public:
    TextureManager(Device* device, AssetManager* assetRepo)
        : m_device(device), m_assetRepo(assetRepo) {}

    Handle LoadTexture(const TextureAssetFormat& path);

  private:
    Device* m_device;
    AssetManager* m_assetRepo;


};
}  // namespace core::render