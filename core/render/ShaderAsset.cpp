#include "ShaderAsset.h"

namespace core::render {

ShaderAsset core::render::ShaderAsset::CreateShaderAsset(wgpu::ShaderModule shaderModule,
                                                         sa::ShaderVisibility shaderStage,
                                                         const std::vector<sa::Binding> bindings) {
    wgpu::ShaderModule vertexModule = nullptr;
    wgpu::ShaderModule fragmentModule = nullptr;
    wgpu::ShaderModule computeModule = nullptr;
    if (static_cast<uint8_t>(shaderStage) & static_cast<uint8_t>(sa::ShaderVisibility::Vertex)) {
        vertexModule = shaderModule;
    }
    if (static_cast<uint8_t>(shaderStage) & static_cast<uint8_t>(sa::ShaderVisibility::Fragment)) {
        fragmentModule = shaderModule;
    }
    if (static_cast<uint8_t>(shaderStage) & static_cast<uint8_t>(sa::ShaderVisibility::Compute)) {
        computeModule = shaderModule;
    }

    return ShaderAsset(vertexModule, fragmentModule, computeModule, shaderStage, std::move(bindings));
}

std::expected<ShaderAsset, Error> ShaderAsset::MergeShaderAsset(const ShaderAsset& a,
                                                                const ShaderAsset& b) {
    if (a.m_shaderStage & b.m_shaderStage)
    {
        return std::unexpected(
            Error::AssetParsing("Cannot merge two ShaderAssets with the same shader stage."));
    }

    wgpu::ShaderModule vertexModule =
        a.m_vertexModule != nullptr ? a.m_vertexModule : b.m_vertexModule;
    wgpu::ShaderModule fragmentModule =
        a.m_fragmentModule != nullptr ? a.m_fragmentModule : b.m_fragmentModule;
    wgpu::ShaderModule computeModule =
        a.m_computeModule != nullptr ? a.m_computeModule : b.m_computeModule;
    sa::ShaderVisibility shaderStage = static_cast<sa::ShaderVisibility>(
        static_cast<uint8_t>(a.m_shaderStage) | static_cast<uint8_t>(b.m_shaderStage));

    const std::vector<sa::Binding>& aBindings = a.m_bindings;
    const std::vector<sa::Binding>& bBindings = b.m_bindings;

    auto aIt = aBindings.begin();
    auto bIt = bBindings.begin();
    std::vector<sa::Binding> mergedBindings;

    while (aIt != aBindings.end() && bIt != bBindings.end()) {
        if (aIt->set == bIt->set && aIt->binding == bIt->binding) {
            if (aIt->resourceType != bIt->resourceType) {
                return std::unexpected(Error::AssetParsing(
                    "Binding Conflict: Same binding index but incompatible resource types!"));
            }
            sa::Binding mergedBinding = *aIt;
            mergedBinding.visibility = static_cast<sa::ShaderVisibility>(
                static_cast<uint8_t>(aIt->visibility) | static_cast<uint8_t>(bIt->visibility));
            mergedBindings.push_back(mergedBinding);
            ++aIt;
            ++bIt;
        } else if (aIt->set < bIt->set ||
                   (aIt->set == bIt->set && aIt->binding < bIt->binding)) {
            mergedBindings.push_back(*aIt);
            ++aIt;
        } else {
            mergedBindings.push_back(*bIt);
            ++bIt;
        }
    }

    while (aIt != aBindings.end()) {
        mergedBindings.push_back(*aIt);
        ++aIt;
    }
    while (bIt != bBindings.end()) {
        mergedBindings.push_back(*bIt);
        ++bIt;
    }

    return ShaderAsset(vertexModule, fragmentModule, computeModule, shaderStage,
                       std::move(mergedBindings));
}


}