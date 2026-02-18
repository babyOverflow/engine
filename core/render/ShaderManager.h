#pragma once

#include "AssetManager.h"
#include "LayoutCache.h"
#include "ShaderAsset.h"
#include "import/ShdrImporter.h"

namespace core::render {

class ShaderManager {
  public:
    ShaderManager(Device* device, AssetManager* assetRepo, LayoutCache* layoutCache);

    ShaderAsset CreateFromShaderSource(const core::importer::ShaderBlob &shaderBlob);
    std::expected<ShaderAsset, Error> MergeShaderAsset(const ShaderAsset& a, const ShaderAsset& b);

    Handle LoadShader(const core::importer::ShaderImportResult& shaderResult);
    AssetView<ShaderAsset> GetShader(const AssetPath& shaderPath);
    AssetView<ShaderAsset> GetShaderAsset(Handle shaderHandle);
    AssetView<ShaderAsset> GetStandardShader() { return m_assetRepo->GetShaderAsset(m_standardShader); }

  private:
    std::array<wgpu::BindGroupLayout, 4> CreateGroupLayouts(
        const core::render::ShaderReflectionData& reflection);
    Device* m_device;
    AssetManager* m_assetRepo;
    LayoutCache* m_layoutCache;
    Handle m_standardShader;

    std::unordered_map<AssetPath, Handle> m_shaderCache;
};
}  // namespace core::render