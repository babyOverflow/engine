#include "Material.h"
#include "render/util.h"

namespace core::render {

void Material::SetVariable(const std::string& name, float value) {
    auto it = m_variableInfo.find(name);
    if (it == m_variableInfo.end()) {
        return;
    }
    size_t offset = it->second;
    std::memcpy(m_variableBufferData.data() + offset, &value, sizeof(float));
}

void Material::SetVariable(const std::string& name, const glm::vec3& value) {
    auto it = m_variableInfo.find(name);
    if (it == m_variableInfo.end()) {
        return;
    }
    size_t offset = it->second;
    std::memcpy(m_variableBufferData.data() + offset, &value, sizeof(glm::vec3));
}

void Material::SetVariable(const std::string& name, const glm::vec4& value) {
    auto it = m_variableInfo.find(name);
    if (it == m_variableInfo.end()) {
        return;
    }
    size_t offset = it->second;
    std::memcpy(m_variableBufferData.data() + offset, &value, sizeof(glm::vec4));
}

MaterialSystem::MaterialSystem(Device* device,
                                 LayoutCache* layoutCache,
                                 PipelineManager* piplineManager)
    : m_device(device), m_layoutCache(layoutCache), m_pipelineManager(piplineManager) {}

void MaterialSystem::RegisterShader(ShaderAsset* shaderAsset) {
    // Implementation would go here
    const auto & bindings = shaderAsset->GetBindings();

    auto wgpuBindings =  util::MapShdrBindToWgpu(bindings);

    std::array<wgpu::BindGroupLayout, 4> bindGroupLayouts{};

    for (size_t i = 0; i < bindGroupLayouts.size(); ++i)     {
        auto group = wgpuBindings.GetGroup(static_cast<uint32_t>(i));
        if (group.size() == 0) {
            continue;
        }
        wgpu::BindGroupLayoutDescriptor bglDesc{};
        bglDesc.entryCount = group.size();
        bglDesc.entries = group.data();
        bindGroupLayouts[i] = m_layoutCache->GetBindGroupLayout(bglDesc);
    }

    auto materialLayout = bindGroupLayouts[kMaterialBindIndex];

    Material material(materialLayout, nullptr);



}

}  // namespace core::render
