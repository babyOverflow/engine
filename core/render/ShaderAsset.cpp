#include <ranges>

#include "ShaderAsset.h"

namespace core::render {

ShaderAsset ShaderAsset::Create(wgpu::ShaderModule shaderModule,
                                std::unique_ptr<ShaderAssetFormat> shaderAssetFormat,
                                ShaderReflection shaderReflection,
    std::vector<std::array<wgpu::BindGroupLayout, 4>> bindGroupLayoutSets) {
    return ShaderAsset(shaderModule, std::move(shaderAssetFormat), std::move(shaderReflection),
                       bindGroupLayoutSets);
}

std::span<const ShaderReflection::Binding> ShaderReflection::GetGroup(uint32_t entryIdx,
                                                                      uint32_t setIdx) const {
    const auto& groups = m_layouts[entryIdx];
    return std::span<const ShaderAssetFormat::Binding>(
        m_shaderAssetFormat->bindings.begin() + (groups[setIdx].offset), groups[setIdx].count);
};

std::span<const ShaderReflection::Binding> ShaderReflection::GetAllBindings() const {
    return m_shaderAssetFormat->bindings;
}

std::span<const MaterialVariableInfo> ShaderReflection::GetMaterialVariableInfos() const {
    // TODO!
    assert(false && "GetMaterialVariableInfos is not implemented");
    return {};
}

std::optional<uint32_t> ShaderReflection::GetEntryPointOffsetByName(const std::string& name) const {
    for (uint32_t i = 0; i < m_shaderAssetFormat->entryPoints.size(); ++i) {
        if (GetNameByIndex(m_shaderAssetFormat->entryPoints[i].nameIdx) == name) {
            return i;
        }
    }
    return std::nullopt;
}

std::span<const ShaderAssetFormat::ShaderParameter> ShaderReflection::GetEntryInput(
    uint32_t entryIdx) const {
    return std::span(m_shaderAssetFormat->parameters.data() +
                         m_shaderAssetFormat->entryPoints[entryIdx].ioStartIndex,
                     m_shaderAssetFormat->entryPoints[entryIdx].ioCount);
}

std::span<const ShaderReflection::Parameter> core::render::ShaderReflection::GetEntryInputByName(const std::string& name) const {
    auto indexOpt = GetEntryPointOffsetByName(name);
    if (indexOpt.has_value()) {
        GetEntryInput(indexOpt.value());
    }

    return std::span<const ShaderReflection::Parameter>();
}

std::string_view ShaderReflection::GetNameByIndex(uint32_t idx) const {
    return m_nameTable[idx];
}

ShaderReflection ShaderReflection::Create(ShaderAssetFormat* shaderAssetFormat) {
    auto nametable = shaderAssetFormat->nameTableData | std::views::split('\0') |
                     std::views::transform([](auto&& s) {
                         return std::string_view(std::ranges::data(s), std::ranges::size(s));
                     }) |
                     std::ranges::to<std::vector>();

    const auto& bindings = shaderAssetFormat->bindings;
    std::vector<std::array<GroupRange, 4>> layouts;
    layouts.reserve(shaderAssetFormat->entryPoints.size());
    for (uint32_t entry = 0; entry < shaderAssetFormat->entryPoints.size(); ++entry) {
        std::array<GroupRange, 4> groups{};
        for (uint32_t j = shaderAssetFormat->entryPoints[entry].bindingStartIndex;
             j < shaderAssetFormat->entryPoints[entry].bindingStartIndex +
                     shaderAssetFormat->entryPoints[entry].bindingCount;
             ++j) {
            const ShaderAssetFormat::Binding& binding = bindings[j];

            if (groups[binding.set].count == 0) {
                groups[binding.set].offset = j;
            }
            groups[binding.set].count++;
        }
        layouts.push_back(groups);
    }
    return ShaderReflection(shaderAssetFormat, std::move(nametable), std::move(layouts));
}

}  // namespace core::render
