#include <ranges>

#include "ShaderAsset.h"

namespace core::render {

ShaderAsset ShaderAsset::Create(wgpu::ShaderModule shaderModule,
                                std::unique_ptr<ShaderAssetFormat> shaderAssetFormat,
                                ShaderReflection shaderReflection,
                                std::array<wgpu::BindGroupLayout, 4> bindGroupLayouts) {

    return ShaderAsset(shaderModule, std::move(shaderAssetFormat), shaderReflection,
                       bindGroupLayouts);
}

const std::span<const ShaderReflection::Binding> ShaderReflection::GetGroup(uint32_t setIdx) const {
    return std::span<const ShaderAssetFormat::Binding>(
        m_shaderAssetFormat->bindings.begin() + (groups[setIdx].offset), groups[setIdx].count);
};

const std::span<const ShaderReflection::Binding> ShaderReflection::GetAllBindings() const {
    return m_shaderAssetFormat->bindings;
}

const std::span<const MaterialVariableInfo> ShaderReflection::GetMaterialVariableInfos() const {
    // TODO!
    assert(false && "GetMaterialVariableInfos is not implemented");
    return {};
}

const std::span<const ShaderReflection::Parameter> ShaderReflection::GetEntryInput(
    const std::string& name) const {
    auto it = std::ranges::find(m_shaderAssetFormat->entryPoints, name, [this](const auto& entry) {
        return GetNameByIndex(entry.nameIdx);
    });
    if (it == m_shaderAssetFormat->entryPoints.end()) {
        return std::span<const ShaderReflection::Parameter>();
    }

    return std::span(m_shaderAssetFormat->parameters.data() + it->ioStartIndex, it->ioCount);
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

    std::array<GroupRange, 4> groups;
    const auto& bindings = shaderAssetFormat->bindings;
    for (size_t i = 0; i < bindings.size(); ++i) {
        const ShaderAssetFormat::Binding& binding = bindings[i];

        uint32_t size = 0;
        if (binding.resourceType == ShaderAssetFormat::ResourceType::UniformBuffer) {
            size = binding.resource.buffer.bufferSize;
        }

        if (groups[binding.set].count == 0) {
            groups[binding.set].offset = i;
        }
        groups[binding.set].count++;
    }
    return ShaderReflection(shaderAssetFormat, std::move(nametable), groups);
}


}  // namespace core::render