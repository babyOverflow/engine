#pragma once
#include <unordered_map>
#include <MaterialAssetFormat.h>

#include "import/Importer.h"

#include "AssetManager.h"
#include "Material.h"
#include "ShaderManager.h"
#include "TextureManager.h"

namespace core::render {

class MaterialManager {
  public:
    MaterialManager() = delete;
    MaterialManager(Device* device,
        AssetManager* assetManager,
                    ShaderManager* shaderManager,
                    TextureManager* textureManager,
                    LayoutCache* layoutCache);
    ~MaterialManager() = default;
    MaterialManager(const MaterialManager&) = delete;
    MaterialManager& operator=(const MaterialManager&) = delete;
    MaterialManager(MaterialManager&&) noexcept = default;
    MaterialManager& operator=(MaterialManager&&) noexcept = default;

    Handle LoadMaterial(const importer::MaterialResult& materialAssetFormat);
    Handle GetMaterialHandle(const AssetPath& assetPath) const {
        auto it = m_materialCache.find(assetPath);
        if (it != m_materialCache.end()) {
            return it->second;
        }
        return Handle();
    }
    AssetView<Material> GetMaterial(Handle handle) {
        return {m_assetManager->GetMaterial(handle).Get(), handle};
    }
    AssetView<Material> GetMaterial(const AssetPath& assetPath) {
        auto it = m_materialCache.find(assetPath);
        if (it == m_materialCache.end()) {
            return AssetView<Material>{};
        }
        Handle handle = it->second;
        return m_assetManager->GetMaterial(handle);
    }

  private:
    Material CreateMaterialFromShader(AssetView<ShaderAsset> shaderAsset);

    Device* m_device;
    AssetManager* m_assetManager;
    TextureManager* m_textureManager;
    ShaderManager* m_shaderManager;
    LayoutCache* m_layoutCache;

    std::unordered_map<AssetPath, Handle> m_materialCache;
};

}  // namespace core::render