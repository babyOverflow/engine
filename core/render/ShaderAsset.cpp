#include "ShaderAsset.h"

namespace core::render {

ShaderAsset ShaderAsset::Create(wgpu::ShaderModule shaderModule,
                                sa::ShaderVisibility shaderStage,
                                ShaderReflectionData reflection,
                                std::array<wgpu::BindGroupLayout, 4> bindGroupLayouts) {
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

    return ShaderAsset(vertexModule, fragmentModule, computeModule, shaderStage, reflection,
                       bindGroupLayouts);
}

std::expected<ShaderReflectionData, Error> ShaderReflectionData::MergeReflectionData(
    const ShaderReflectionData& a,
    const ShaderReflectionData& b) {
    using sa = core::ShaderAssetFormat;

    ShaderReflectionData mergedData;
    mergedData.bindings.reserve(a.bindings.size());
    auto aIt = a.bindings.begin();
    auto bIt = b.bindings.begin();

    while (aIt != a.bindings.end() && bIt != b.bindings.end()) {
        if (aIt->set == bIt->set && aIt->binding == bIt->binding) {
            if (aIt->resourceType != bIt->resourceType) {
                return std::unexpected(Error::AssetParsing(
                    "Binding Conflict: Same binding index but incompatible resource types!"));
            }
            BindingInfo mergedBinding = *aIt;
            mergedBinding.visibility = static_cast<sa::ShaderVisibility>(
                static_cast<uint8_t>(aIt->visibility) | static_cast<uint8_t>(bIt->visibility));
            mergedData.bindings.push_back(mergedBinding);
            ++aIt;
            ++bIt;
        } else if (aIt->set < bIt->set || (aIt->set == bIt->set && aIt->binding < bIt->binding)) {
            mergedData.bindings.push_back(*aIt);
            ++aIt;
        } else {
            mergedData.bindings.push_back(*bIt);
            ++bIt;
        }
    }

    while (aIt != a.bindings.end()) {
        mergedData.bindings.push_back(*aIt);
        ++aIt;
    }
    while (bIt != b.bindings.end()) {
        mergedData.bindings.push_back(*bIt);
        ++bIt;
    }

    mergedData.groups = {};
    for (uint32_t i = 0; i < mergedData.bindings.size(); ++i) {
        const auto& binding = mergedData.bindings[i];

        if (mergedData.groups[binding.set].count == 0) {
            mergedData.groups[binding.set].offset = i;
        }
        mergedData.groups[binding.set].count++;
    }
    // TODO!(Now mergedData.materialVariables parsing logic is not implemented. it should be append
    // after that)

    return mergedData;
}

}  // namespace core::render