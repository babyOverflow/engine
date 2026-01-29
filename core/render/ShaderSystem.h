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

  private:
    std::array<wgpu::BindGroupLayout, 4> CreateGroupLayouts(
        const core::render::ShaderReflectionData& reflection);
    Device* m_device;
    AssetManager* m_assetRepo;
    LayoutCache* m_layoutCache;
};
}  // namespace core::render