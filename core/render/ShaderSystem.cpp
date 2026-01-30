#include "ShaderSystem.h"
#include <ranges>
#include "asset/StandardPBR_FS.h"
#include "asset/StandardPBR_VS.h"
#include "util.h"

namespace core::render {

std::array<wgpu::BindGroupLayout, 4> ShaderSystem::CreateGroupLayouts(
    const core::render::ShaderReflectionData& reflection) {
    std::array<wgpu::BindGroupLayout, 4> bindGroupLayouts;

    for (uint32_t i = 0; i < 4; ++i) {
        std::span<const BindingInfo> bindings = reflection.GetGroup(i);
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

ShaderSystem::ShaderSystem(Device* device, AssetManager* assetRepo, LayoutCache* layoutCache)
    : m_device(device), m_assetRepo(assetRepo), m_layoutCache(layoutCache) {
    auto vtxBlobOrError =
        ShaderAssetFormat::LoadFromMemory(kStandardPBR_VS_Data).and_then([&](const auto shdr) {
            return core::importer::ShdrImporter::ShdrImport(m_device, shdr);
        });
    auto frgBlobOrError =
        ShaderAssetFormat::LoadFromMemory(kStandardPBR_FS_Data).and_then([&](const auto& shdr) {
            return core::importer::ShdrImporter::ShdrImport(m_device, shdr);
        });

    if (!vtxBlobOrError.has_value()) {
        assert(false && "Failed to load standard PBR vertex shader");
    }
    if (!frgBlobOrError.has_value()) {
        assert(false && "Failed to load standard PBR fragment shader");
    }

    core::importer::ShaderBlob& vtxBlob = vtxBlobOrError.value();
    core::importer::ShaderBlob& frgBlob = frgBlobOrError.value();

    // ShaderSystem's layout caching logic relies on reflection data. Merging reflections reduces
    // unused layout cache entries.
    auto reflectionOrError =
        render::ShaderReflectionData::MergeReflectionData(vtxBlob.reflection, frgBlob.reflection);
    if (!reflectionOrError.has_value()) {
        assert(false && "Failed to load standard PBR shader");
    }
    vtxBlob.reflection = reflectionOrError.value();
    vtxBlob.reflection.entryShaderStage = ShaderAssetFormat::ShaderVisibility::Vertex;
    frgBlob.reflection = reflectionOrError.value();
    frgBlob.reflection.entryShaderStage = ShaderAssetFormat::ShaderVisibility::Fragment;

    render::ShaderAsset vtxShader = CreateFromShaderSource(vtxBlob);
    render::ShaderAsset frgShader = CreateFromShaderSource(frgBlob);

    auto renderShaderOrError = MergeShaderAsset(vtxShader, frgShader);

    if (!renderShaderOrError.has_value())
    {
        assert(false && "Failed to load standard PBR shader");
    }

    m_standardShader = m_assetRepo->StoreShaderAsset(std::move(renderShaderOrError.value()));
}

ShaderAsset ShaderSystem::CreateFromShaderSource(const core::importer::ShaderBlob& shaderBlob) {
    const auto& reflection = shaderBlob.reflection;

    std::array<wgpu::BindGroupLayout, 4> bindGroupLayouts = CreateGroupLayouts(reflection);

    return ShaderAsset::Create(shaderBlob.shaderModule, reflection.entryShaderStage, reflection,
                               bindGroupLayouts);
}

std::expected<ShaderAsset, Error> ShaderSystem::MergeShaderAsset(const ShaderAsset& a,
                                                                 const ShaderAsset& b) {
    using Fmt = ShaderAssetFormat;
    if (a.m_shaderStage & b.m_shaderStage) {
        return std::unexpected(
            Error::AssetParsing("Cannot merge two ShaderAssets with the same shader stage."));
    }

    wgpu::ShaderModule vertexModule =
        a.m_vertexModule != nullptr ? a.m_vertexModule : b.m_vertexModule;
    wgpu::ShaderModule fragmentModule =
        a.m_fragmentModule != nullptr ? a.m_fragmentModule : b.m_fragmentModule;
    wgpu::ShaderModule computeModule =
        a.m_computeModule != nullptr ? a.m_computeModule : b.m_computeModule;
    Fmt::ShaderVisibility shaderStage = static_cast<Fmt::ShaderVisibility>(
        static_cast<uint8_t>(a.m_shaderStage) | static_cast<uint8_t>(b.m_shaderStage));

    auto reflectionOrError =
        ShaderReflectionData::MergeReflectionData(a.GetReflection(), b.GetReflection());

    if (reflectionOrError.has_value() == false) {
        return std::unexpected(reflectionOrError.error());
    }

    const auto& reflection = reflectionOrError.value();
    std::array<wgpu::BindGroupLayout, 4> bindGroupLayouts = CreateGroupLayouts(reflection);

    return ShaderAsset(vertexModule, fragmentModule, computeModule, shaderStage, reflection,
                       bindGroupLayouts);
}

AssetView<ShaderAsset> ShaderSystem::GetShaderAsset(Handle shaderHandle) {
    return m_assetRepo->GetShaderAsset(shaderHandle);
}
}  // namespace core::render