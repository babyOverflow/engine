#pragma once
#include <expected>

#include <Common.h>
#include <MeshAssetFormat.h>
#include <ModelAssetFormat.h>
#include <render/MeshManager.h>
#include <render/Model.h>
#include <render/TextureManager.h>

namespace loader {
class GLTFLoader {
  public:
    GLTFLoader(core::AssetManager* assetManager,
               core::render::TextureManager* textureManager,
               core::render::MeshManager* meshManager)
        : m_assetManager(assetManager),
          m_textureManger(textureManager),
          m_meshManager(meshManager) {}

    std::expected<core::Handle, core::Error> LoadModel(const std::string& path);

  private:
    core::AssetManager* m_assetManager = nullptr;
    core::render::TextureManager* m_textureManger = nullptr;
    core::render::MeshManager* m_meshManager = nullptr;

    std::unordered_map<std::string, core::Handle> m_modelCache;
};
}  // namespace loader