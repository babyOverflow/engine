#include <cassert>

#include "BindGroupFactory.h"
#include "BindGroupManager.h"

core::render::BindGroupManager::BindGroupManager(Device* device,
                                                 ShaderManager* shaderManager,
                                                 MaterialManager* materialManager)
    : m_device(device),
      m_shaderManager(shaderManager),
      m_materialManager(materialManager) {}

wgpu::BindGroup core::render::BindGroupManager::GetBindGroup(Handle materialHandle) {
    // TODO!(Sunghyun) replace it with debug assertion after finish logging system and macros
    assert(materialHandle.index < m_materialPassBindGroups.size());
    assert(m_materialPassBindGroups[materialHandle.index].Get() != nullptr);
    return m_materialPassBindGroups[materialHandle.index];
}

void core::render::BindGroupManager::UpdateBindGroup(Handle materialHandle) {
    const AssetView<Material> material = m_materialManager->GetMaterial(materialHandle);
    const uint8_t technique = material->GetActiveTechniqueID();

    wgpu::BindGroupLayout bindGroupLayout = m_shaderManager->GetMaterialBindGroupLayout(technique);
    std::span<ShaderAssetFormat::Binding> bindings =
        m_shaderManager->GetMaterialBindGroupInfo(technique);

    MaterialProvider provider{
        .material = material,
    };
    wgpu::BindGroup bindgroup =
        BindGroupFactory::Create(m_device, bindGroupLayout, bindings, provider);
    uint32_t index = materialHandle.index;
    if (m_materialPassBindGroups.size() <= index) {
        m_materialPassBindGroups.resize(index + 1);
    }
    m_materialPassBindGroups[index] = bindgroup;
}
