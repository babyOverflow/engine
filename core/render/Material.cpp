#include "Material.h"
#include "render/util.h"

namespace core::render {



void Material::UpdateUniform() {
    m_device->WriteBuffer(m_uniformBuffer, 0, m_cpuVariableBufferData.data(),
                          m_cpuVariableBufferData.size());
    m_isDirty = false;
}

void Material::RebuildBindGroup() {
    const auto& bindGroupLayout = m_shaderView->GetBindGroupLayout(kSetNumberMaterial);
    const auto& reflection = m_shaderView->GetReflection();
    const auto& entryInfos = reflection.GetGroup(kSetNumberMaterial);
    const auto& uniformInfo = reflection.GetMaterialVariableInfos();

    std::vector<wgpu::BindGroupEntry> entries;
    for (uint32_t i = 0; i < entryInfos.size(); ++i) {
        const auto& entryInfo = entryInfos[i];
        switch (entryInfo.resourceType) {
            case core::ShaderAssetFormat::ResourceType::UniformBuffer: {
                wgpu::BindGroupEntry bufferEntry{
                    .binding = entryInfo.binding,
                    .buffer = m_uniformBuffer,
                    .offset = 0,
                    .size = reflection.materialUniformSize,
                };
                entries.push_back(bufferEntry);
                break;
            }
            case core::ShaderAssetFormat::ResourceType::Texture: {
                auto it = m_textures.find(entryInfo.id);
                if (it == m_textures.end()) {
                    it = m_textures.find(ToPropertyID(Texture::kDefaultTexture.value));
                }
                const auto& texture = it->second;
                wgpu::BindGroupEntry textureEntry{
                    .binding = entryInfo.binding,
                    .textureView = texture->GetView(),
                };
                entries.push_back(textureEntry);
                break;
            }
            case core::ShaderAssetFormat::ResourceType::Sampler: {
                assert(false && "Sampler in material is not supported yet.");
            }
            default:
                assert(false && "not supported resource type.");

                break;
        }
    }

    wgpu::BindGroupDescriptor bindGroupDesc{
        .layout = bindGroupLayout,
        .entryCount = static_cast<uint32_t>(entries.size()),
        .entries = entries.data(),
    };
    m_bindGroup = m_device->CreateBindGroup(bindGroupDesc);
}

std::expected<wgpu::BindGroup, Error> Material::GetBindGroup() {
    if (m_bindGroup == nullptr) {
        RebuildBindGroup();
    }
    return m_bindGroup;
}

}  // namespace core::render
