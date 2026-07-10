#include <ranges>

#include "ShaderAsset.h"

namespace core::render {

ShaderAsset ShaderAsset::Create(wgpu::ShaderModule shaderModule,
                                std::unique_ptr<ShaderAssetFormat> shaderAssetFormat,
                                ShaderReflection shaderReflection,
                                std::array<wgpu::BindGroupLayout, 4> bindGroupLayoutSets) {
    return ShaderAsset(shaderModule, std::move(shaderAssetFormat), std::move(shaderReflection),
                       bindGroupLayoutSets);
}

std::span<const ShaderReflection::Binding> ShaderReflection::GetGroup(uint32_t setIdx) const {
    return std::span<const ShaderAssetFormat::Binding>(
        m_shaderAssetFormat->bindings.data() + (m_layouts[setIdx].offset),
        m_layouts[setIdx].count);
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

std::span<const ShaderAssetFormat::ShaderParameter> core::render::ShaderReflection::GetEntryIO(
    uint32_t entryIdx) const {
    return std::span(m_shaderAssetFormat->parameters.data() +
                         m_shaderAssetFormat->entryPoints[entryIdx].ioStartIndex,
                     m_shaderAssetFormat->entryPoints[entryIdx].ioCount);
}

std::span<const ShaderReflection::Parameter> core::render::ShaderReflection::GetEntryInputByName(
    const std::string& name) const {
    auto indexOpt = GetEntryPointOffsetByName(name);
    if (indexOpt.has_value()) {
        GetEntryIO(indexOpt.value());
    }

    return std::span<const ShaderReflection::Parameter>();
}

std::string_view ShaderReflection::GetNameByIndex(uint32_t idx) const {
    return m_nameTable[idx];
}

std::string_view ShaderReflection::GetPassName() const {
    if (m_shaderAssetFormat->header.passNameIndex != ShaderAssetFormat::kInvalidIdx<uint16_t>) {
        return m_nameTable[m_shaderAssetFormat->header.passNameIndex];
    } else {
        return "";
    }
}

std::string_view ShaderReflection::GetMaterialTechName() const {
    if (m_shaderAssetFormat->header.materialNameIndex != ShaderAssetFormat::kInvalidIdx<uint16_t>) {
        return m_nameTable[m_shaderAssetFormat->header.materialNameIndex];
    } else {
        return "";
    }
}

ShaderReflection ShaderReflection::Create(ShaderAssetFormat* shaderAssetFormat) {
    auto nametable = shaderAssetFormat->nameTableData | std::views::split('\0') |
                     std::views::transform([](auto&& s) {
                         return std::string_view(std::ranges::data(s), std::ranges::size(s));
                     }) |
                     std::ranges::to<std::vector>();

    const auto& bindings = shaderAssetFormat->bindings;
    std::array<GroupRange, 4> groups{};
    for (uint32_t j = 0; j < bindings.size(); ++j) {
        const ShaderAssetFormat::Binding& binding = bindings[j];
        if (binding.set < 4) {
            if (groups[binding.set].count == 0) {
                groups[binding.set].offset = j;
            }
            groups[binding.set].count++;
        }
    }

    return ShaderReflection(shaderAssetFormat, std::move(nametable), std::move(groups));
}

}  // namespace core::render
