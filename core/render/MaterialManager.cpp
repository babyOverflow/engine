#include "MaterialManager.h"
#include <ranges>

namespace core::render {

MaterialManager::MaterialManager(Device* device,
                                 AssetManager* assetManager,
                                 ShaderManager* shaderManager,
                                 TextureManager* textureManager,
                                 LayoutCache* layoutCache)
    : m_device(device),
      m_assetManager(assetManager),
      m_layoutCache(layoutCache),
      m_shaderManager(shaderManager),
      m_textureManager(textureManager) {}

Handle MaterialManager::LoadMaterial(const importer::MaterialResult& materialResult) {
    if (auto it = m_materialCache.find(materialResult.assetPath); it != m_materialCache.end()) {
        return it->second;
    }
    const auto& materialAsset = materialResult.materialAsset;
    auto shaderAssetView = m_shaderManager->GetShader(materialAsset.shaderName);
    if (!shaderAssetView.IsValid()) {
        return Handle();
    }
    auto material = CreateMaterialFromShader(shaderAssetView);

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
    return handle;
}

Material MaterialManager::CreateMaterialFromShader(AssetView<ShaderAsset> shaderAsset) {
    if (!shaderAsset->IsValidRenderShader()) {
        return Material();
    }

    const ShaderReflectionData& bindGroupInfos = shaderAsset->GetReflection();

    const auto variables = bindGroupInfos.GetMaterialVariableInfos();

    size_t bufferSize = bindGroupInfos.materialUniformSize;
    Material material = [&]() {
        if (bufferSize) {
            std::unordered_map<PropertyId, Material::VariableInfo> variableInfo;
            for (const auto& var : variables) {
                variableInfo[var.id] = {
                    .offset = var.offset,
                    .size = var.size,
                };
            }

            wgpu::BufferDescriptor bufferDesc{
                .usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst,
                .size = bufferSize,
            };
            wgpu::Buffer buffer = m_device->CreateBuffer(bufferDesc);
            std::vector<std::byte> emptyData(bufferSize, std::byte{0});
            return Material(m_device, shaderAsset, std::move(buffer), std::move(emptyData),
                            std::move(variableInfo));
        } else {
            return Material(m_device, shaderAsset);
        }
    }();

    material.SetTexture(ToPropertyID(Texture::kDefaultTexture.value),
                        m_textureManager->GetTexture(Texture::kDefaultTexture));
    return material;
}

}  // namespace core::render