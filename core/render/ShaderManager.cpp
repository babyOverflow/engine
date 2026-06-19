#include "ShaderManager.h"
#include <ranges>
#include "asset/StandardPBR.h"
#include "util.h"

namespace core::render {

void ShaderLookupTable::Initialize(AssetView<ShaderAsset> shaderFallBack) {
    shaderFallback = shaderFallBack;
    for (auto& pass : m_table) {
        pass.fill(shaderFallBack.handle);
    }
}

std::array<wgpu::BindGroupLayout, 4> ShaderManager::CreateGroupLayouts(
    const core::render::ShaderReflection& reflection) {
    std::array<wgpu::BindGroupLayout, 4>;

    std::array<wgpu::BindGroupLayout, 4> bindGroupLayouts = {};
    for (uint32_t set = 0; set < 4; ++set) {
        const std::span<const ShaderAssetFormat::Binding> bindings = reflection.GetGroup(set);

        std::vector<wgpu::BindGroupLayoutEntry> entries =
            bindings | std::views::transform(util::MapBindingInfoToWgpu) |
            std::ranges::to<std::vector>();

        wgpu::BindGroupLayoutDescriptor desc{
            .entryCount = entries.size(),
            .entries = entries.data(),
        };
        #ifndef NDEBUG
        std::string bindGroupLabel;
        bindGroupLabel += reflection.GetPassName();
        bindGroupLabel += reflection.GetMaterialTechName();
        bindGroupLabel += " Group: " + std::to_string(set);
        desc.label = std::string_view(bindGroupLabel);
        #endif
        reflection.GetPassName();
        bindGroupLayouts[set] = m_layoutCache->GetBindGroupLayout(desc);
    }
    return bindGroupLayouts;
}

ShaderManager::ShaderManager(Device* device,
                             AssetManager* assetRepo,
                             LayoutCache* layoutCache,
                             PassManager* passManager,
                             MaterialManager* materialManager)
    : m_device(device),
      m_assetRepo(assetRepo),
      m_layoutCache(layoutCache),
      m_passManager(passManager),
      m_materialManager(materialManager) {
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

    return ShaderAsset::Create(shaderModule, std::move(shaderAssetFormat), std::move(reflection),
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

    std::string_view passName = shaderAsset.GetReflection().GetPassName();
    std::string_view materialTechName = shaderAsset.GetReflection().GetMaterialTechName();
    uint8_t passId = m_passManager->GetPassID(passName);
    uint8_t materialTechId = m_materialManager->GetTechniqueID(materialTechName);

    if (!m_techniqueBindGroups[materialTechId]) {
        m_techniqueBindGroups[materialTechId] = shaderAsset.GetBindGroupLayout(BindSlot::Material);
        std::span<const ShaderReflection::Binding> bindings =
            shaderAsset.GetReflection().GetGroup(BindSlot::Material);
        m_techniqueBindInfo[materialTechId] =
            std::vector<ShaderReflection::Binding>(bindings.begin(), bindings.end());
    }


    Handle handle = m_assetRepo->StoreShaderAsset(std::move(shaderAsset));
    m_shaderLookupTable.m_table[passId][materialTechId] = handle;
    m_shaderCache[shaderResult.assetPath] = handle;
    return handle;
}

}  // namespace core::render