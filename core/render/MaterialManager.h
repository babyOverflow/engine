#pragma once
#include <MaterialAssetFormat.h>
#include <unordered_map>

#include "import/Importer.h"

#include "AssetManager.h"
#include "IRenderPass.h"
#include "Material.h"
#include "TextureManager.h"

namespace core::render {

class MaterialManager;

class MaterialMutator {
  public:
    MaterialMutator(MaterialManager* manager, AssetView<Material> material)
        : m_materialManager(manager), m_material(material) {}

    void SetTexture(const std::string& name, AssetView<Texture> texture);
    void SetTexture(PropertyId id, AssetView<Texture> texture);

    template <ValidMaterialVariableType T>
    void SetVariable(PropertyId id, const T value);

    template <ValidMaterialVariableType T>
    void SetVariable(const std::string& name, const T value);

  private:
    MaterialManager* m_materialManager;
    AssetView<Material> m_material;
};

class MaterialManager {
  public:
    MaterialManager() = delete;
    MaterialManager(Device* device,
                    AssetManager* assetManager,
                    TextureManager* textureManager,
                    PassManager* passManager,
                    LayoutCache* layoutCache);
    ~MaterialManager() = default;
    MaterialManager(const MaterialManager&) = delete;
    MaterialManager& operator=(const MaterialManager&) = delete;
    MaterialManager(MaterialManager&&) noexcept = default;
    MaterialManager& operator=(MaterialManager&&) noexcept = default;

    static inline std::string kEmptyMaterialName = "EmptyMaterial";
    static constexpr uint32_t kEmptyMaterialTechniqe = 0;

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

    void AddDirtyMaterial(Handle materialHandle);
    std::span<Handle> GetDirtyMaterials() { return m_dirtymaterials; }
    void ClearDirties();

    uint32_t GetTechniqueID(std::string_view techniequeName);

    MaterialMutator GetMaterialMutator(Handle materialHandle);

  private:
    Device* m_device;
    AssetManager* m_assetManager;
    TextureManager* m_textureManager;
    PassManager* m_passManager;
    LayoutCache* m_layoutCache;

    std::unordered_map<AssetPath, Handle> m_materialCache;

    std::unordered_map<std::string, uint32_t, transparent_string_hash, std::equal_to<>>
        m_nameToTechniqueIdCache;

    std::vector<Handle> m_dirtymaterials;
};

}  // namespace core::render