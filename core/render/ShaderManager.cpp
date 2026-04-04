#include "ShaderManager.h"
#include <ranges>
#include "asset/StandardPBR.h"
#include "util.h"

namespace core::render {

std::array<wgpu::BindGroupLayout, 4> ShaderManager::CreateGroupLayouts(
    const core::render::ShaderReflection& reflection) {
    std::array<wgpu::BindGroupLayout, 4> bindGroupLayouts;

    for (uint32_t i = 0; i < 4; ++i) {
        const std::span<const ShaderAssetFormat::Binding> bindings = reflection.GetGroup(i);
        std::vector<wgpu::BindGroupLayoutEntry> entries =
            bindings | std::views::transform(util::MapBindingInfoToWgpu) |
            std::ranges::to<std::vector>();

        wgpu::BindGroupLayoutDescriptor desc{
            .entryCount = entries.size(),
            .entries = entries.data(),
        };
        bindGroupLayouts[i] = m_layoutCache->GetBindGroupLayout(desc);
    }
    return bindGroupLayouts;
}

ShaderManager::ShaderManager(Device* device, AssetManager* assetRepo, LayoutCache* layoutCache)
    : m_device(device), m_assetRepo(assetRepo), m_layoutCache(layoutCache) {
    auto blobOrError = ShaderAssetFormat::LoadFromMemory(kStandardPBR_Data);

    if (!blobOrError.has_value()) {
        assert(false && "Failed to load standard PBR shader");
    }

    ShaderAssetFormat& blob = blobOrError.value();

    render::ShaderAsset shader = CreateFromShaderSource(std::move(blob));

    m_standardShader = m_assetRepo->StoreShaderAsset(std::move(shader));
}

ShaderAsset ShaderManager::CreateFromShaderSource(ShaderAssetFormat&& shaderAsset) {
    std::string_view wgslCode(reinterpret_cast<const char*>(shaderAsset.code.data()),
                              shaderAsset.code.size());
    wgpu::ShaderModule shaderModule = m_device->CreateShaderModuleFromWGSL(wgslCode);

    auto shaderAssetFormat = std::make_unique<ShaderAssetFormat>(std::move(shaderAsset));
    ShaderReflection reflection = ShaderReflection::Create(shaderAssetFormat.get());

    std::array<wgpu::BindGroupLayout, 4> bindGroupLayouts = CreateGroupLayouts(reflection);

    return ShaderAsset::Create(shaderModule, std::move(shaderAssetFormat), reflection,
                               bindGroupLayouts);
}

AssetView<ShaderAsset> ShaderManager::GetShaderAsset(Handle shaderHandle) {
    return m_assetRepo->GetShaderAsset(shaderHandle);
}

AssetView<ShaderAsset> ShaderManager::GetShader(const AssetPath& shaderPath) {
    const auto it = m_shaderCache.find(shaderPath);
    if (it != m_shaderCache.end()) {
        return m_assetRepo->GetShaderAsset(it->second);
    }
    return m_assetRepo->GetShaderAsset(m_standardShader);
}

Handle ShaderManager::LoadShader(core::importer::ShaderImportResult&& shaderResult) {
    auto it = m_shaderCache.find(shaderResult.assetPath);
    if (it != m_shaderCache.end()) {
        return it->second;
    }
    core::render::ShaderAsset shaderAsset =
        CreateFromShaderSource(std::move(shaderResult.shaderAsset));
    Handle handle = m_assetRepo->StoreShaderAsset(std::move(shaderAsset));
    m_shaderCache[shaderResult.assetPath] = handle;
    return handle;
}

}  // namespace core::render