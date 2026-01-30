#pragma once

#include "AssetManager.h"
#include "LayoutCache.h"
#include "ShaderAsset.h"
#include "import/ShdrImporter.h"

namespace core::render {

class ShaderSystem {
  public:
    ShaderSystem(Device* device, AssetManager* assetRepo, LayoutCache* layoutCache);

    ShaderAsset CreateFromShaderSource(const core::importer::ShaderBlob &shaderBlob);
    std::expected<ShaderAsset, Error> MergeShaderAsset(const ShaderAsset& a, const ShaderAsset& b);

    Handle LoadShader(const std::string& shaderPath);
    AssetView<ShaderAsset> GetShaderAsset(Handle shaderHandle);
    AssetView<ShaderAsset> GetStandardShader() { return m_assetRepo->GetShaderAsset(m_standardShader); }

  private:
    std::array<wgpu::BindGroupLayout, 4> CreateGroupLayouts(
        const core::render::ShaderReflectionData& reflection);
    Device* m_device;
    AssetManager* m_assetRepo;
    LayoutCache* m_layoutCache;
    Handle m_standardShader;
};
}  // namespace core::render