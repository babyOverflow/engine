#pragma once

#include "AssetManager.h"
#include "IRenderPass.h"
#include "LayoutCache.h"
#include "MaterialManager.h"
#include "ShaderAsset.h"
#include "import/ShdrImporter.h"

namespace core::render {

struct ShaderLookupTable {
    static constexpr uint32_t MAX_PASSES = 32;
    static constexpr uint32_t MAX_MATERIAL_TECHS = 256;  // 2^8 boundary
    AssetView<ShaderAsset> shaderFallback;

    void Initialize(AssetView<ShaderAsset> shaderFallBack);

    [[nodiscard]] inline Handle GetShader(uint8_t passId, uint8_t materialTechId) const {
        if (passId >= MAX_PASSES || materialTechId >= MAX_MATERIAL_TECHS) {
            return Handle{.index = 0, .generation = Handle::kInvalidGen};
        }
        return m_table[passId][materialTechId];
    }
    std::array<std::array<Handle, MAX_MATERIAL_TECHS>, MAX_PASSES> m_table;
};

class ShaderManager {
  public:
    ShaderManager(Device* device,
                  AssetManager* assetRepo,
                  LayoutCache* layoutCache,
                  PassManager* passManager,
                  MaterialManager* materialManager);

    ShaderAsset CreateFromShaderSource(ShaderAssetFormat&& shaderAsset);

    Handle LoadShader(core::importer::ShaderImportResult&& shaderResult);
    Handle GetShaderHandle(uint32_t passId, uint32_t materialTechniqueId) {
        return m_shaderLookupTable.GetShader(passId, materialTechniqueId);
    }
    AssetView<ShaderAsset> GetShader(const AssetPath& shaderPath);
    AssetView<ShaderAsset> GetShaderAsset(Handle shaderHandle);
    AssetView<ShaderAsset> GetStandardShader() {
        return m_assetRepo->GetShaderAsset(m_standardShader);
    }

    wgpu::BindGroupLayout GetMaterialBindGroupLayout(uint8_t techniqueId) {
        return m_techniqueBindGroups[techniqueId];
    }

    std::span<core::ShaderAssetFormat::Binding> GetMaterialBindGroupInfo(uint8_t techniqueId) {
        return m_techniqueBindInfo[techniqueId];
    }

  private:
    std::array<wgpu::BindGroupLayout, 4> CreateGroupLayouts(
        const core::render::ShaderReflection& reflection);
    Device* m_device;
    AssetManager* m_assetRepo;
    LayoutCache* m_layoutCache;
    Handle m_standardShader;
    PassManager* m_passManager;
    MaterialManager* m_materialManager;

    std::unordered_map<AssetPath, Handle> m_shaderCache;
    std::array<wgpu::BindGroupLayout, 255> m_techniqueBindGroups{nullptr};
    std::array<std::vector<core::ShaderAssetFormat::Binding>, 255> m_techniqueBindInfo;

    ShaderLookupTable m_shaderLookupTable;
};
}  // namespace core::render