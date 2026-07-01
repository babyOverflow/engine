#include "MaterialManager.h"
#include <ranges>

namespace core::render {

void MaterialMutator::SetTexture(const std::string& name, AssetView<Texture> texture) {
    m_material->SetTexture(name, texture);
    if (!m_material->IsDirty()) {
        m_materialManager->AddDirtyMaterial(m_material.handle);
        m_material->m_isDirty = true;
    }
}

void MaterialMutator::SetTexture(PropertyId id, AssetView<Texture> texture) {
    m_material->SetTexture(id, texture);
    if (!m_material->IsDirty()) {
        m_materialManager->AddDirtyMaterial(m_material.handle);
        m_material->m_isDirty = true;
    }
}

MaterialManager::MaterialManager(Device* device,
                                 AssetManager* assetManager,
                                 TextureManager* textureManager,
                                 PassManager* passManager,
                                 LayoutCache* layoutCache)
    : m_device(device),
      m_assetManager(assetManager),
      m_layoutCache(layoutCache),
      m_textureManager(textureManager),
      m_passManager(passManager) {

    m_nameToTechniqueIdCache[kEmptyMaterialName] = kEmptyMaterialTechniqe;
    //importer::MaterialResult defaultMaterialResult{
    //    .materialAsset = MaterialAssetFormat{},
    //    .assetPath = AssetPath{"virtual://material/default"},
    //};

    //LoadMaterial(defaultMaterialResult);
}

Handle MaterialManager::LoadMaterial(const importer::MaterialResult& materialResult) {
    if (auto it = m_materialCache.find(materialResult.assetPath); it != m_materialCache.end()) {
        return it->second;
    }
    const auto& materialAsset = materialResult.materialAsset;

    Material material(m_device);

    material.SetTechniqueID(GetTechniqueID(materialAsset.materialTechnique));

    for (const auto& [slotName, texturePath] : materialAsset.textures) {
        auto textureView = m_textureManager->GetTexture(texturePath);

        PropertyId textureId = ToPropertyID(slotName);
        if (!textureView.IsValid()) {
            material.SetTexture(textureId, m_textureManager->GetTexture(Texture::kDefaultTexture));
        } else {
            material.SetTexture(textureId, textureView);
        }
    }

    Handle handle = m_assetManager->StoreMaterial(std::move(material));
    m_materialCache[materialResult.assetPath] = handle;
    m_dirtymaterials.push_back(handle);
    return handle;
}

uint32_t MaterialManager::GetTechniqueID(std::string_view techniequeName) {
    auto it = m_nameToTechniqueIdCache.find(techniequeName);
    if (it != m_nameToTechniqueIdCache.end()) {
        return it->second;
    }
    // If technique name is not found, assign a new ID
    uint32_t newId = static_cast<uint32_t>(m_nameToTechniqueIdCache.size());
    std::string key(techniequeName);
    m_nameToTechniqueIdCache[key] = newId;
    return newId;
}

void MaterialManager::AddDirtyMaterial(Handle materialHandle) {
    m_dirtymaterials.push_back(materialHandle);
}

void MaterialManager::ClearDirties() {
    for (auto handle : m_dirtymaterials) {
        AssetView<Material> material = GetMaterial(handle);
        material->m_isDirty = false;
    }
    m_dirtymaterials.clear();
}

MaterialMutator MaterialManager::GetMaterialMutator(Handle materialHandle) {
    return MaterialMutator(this, GetMaterial(materialHandle));
}

}  // namespace core::render