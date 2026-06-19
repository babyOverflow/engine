#include <cassert>

#include "BindGroupManager.h"

core::render::BindGroupManager::BindGroupManager(Device* device,
                                                 LayoutCache* layoutCache,
                                                 ShaderManager* shaderManager,
                                                 MaterialManager* materialManager)
    : m_device(device),
      m_layoutCache(layoutCache),
      m_shaderManager(shaderManager),
      m_materialManager(materialManager) {}

wgpu::BindGroup core::render::BindGroupManager::GetBindGroup(Handle materialHandle) {
    return m_materialPassBindGroups[materialHandle.index];
}

void core::render::BindGroupManager::UpdateBindGroup(Handle materialHandle) {
    const AssetView<Material> material = m_materialManager->GetMaterial(materialHandle);
    const uint8_t technique = material->GetActiveTechniqueID();

    wgpu::BindGroupLayout bindGroupLayout = m_shaderManager->GetMaterialBindGroupLayout(technique);
    std::span<ShaderAssetFormat::Binding> bindings =
        m_shaderManager->GetMaterialBindGroupInfo(technique);
    std::vector<wgpu::BindGroupEntry> bindGroupEntries;

    for (uint32_t i = 0; i < bindings.size(); ++i) {
        const ShaderReflection::Binding& bindingInfo = bindings[i];
        switch (bindingInfo.resourceType) {
            case core::ShaderAssetFormat::ResourceType::UniformBuffer: {
                assert(false && "Currently uniform variable is not supported");
                wgpu::BindGroupEntry bufferEntry{
                    //.binding = bindingInfo.binding,
                    //.buffer = m_uniformBuffer,
                    //.offset = 0,
                    //.size = reflection.materialUniformSize,
                };
                bindGroupEntries.push_back(bufferEntry);
                break;
            }
            case core::ShaderAssetFormat::ResourceType::Texture: {
                AssetView<Texture> texture = material->GetTexture(bindingInfo.id);
                if (!texture.IsValid()) {
                    // assign default fallback texture;
                }
                wgpu::BindGroupEntry textureEntry{
                    .binding = bindingInfo.binding,
                    .textureView = texture->GetView(),
                };
                bindGroupEntries.push_back(textureEntry);
                break;
            }
            case core::ShaderAssetFormat::ResourceType::Sampler: {
                wgpu::BindGroupEntry samplerEntry{
                    .binding = bindingInfo.binding,
                    .sampler = material->GetSampler(),
                };
                bindGroupEntries.push_back(samplerEntry);
                break;
            }
            default:
                assert(false && "not supported resource type.");

                break;
        }
    }

    wgpu::BindGroupDescriptor bindGroupDesc{
        .layout = bindGroupLayout,
        .entryCount = static_cast<uint32_t>(bindGroupEntries.size()),
        .entries = bindGroupEntries.data(),
    };
    wgpu::BindGroup bindgroup = m_device->CreateBindGroup(bindGroupDesc);
    uint32_t index = materialHandle.index;
    if (m_materialPassBindGroups.size() <= index) {
        m_materialPassBindGroups.resize(index + 1);
    }
    m_materialPassBindGroups[index] = bindgroup;
}

