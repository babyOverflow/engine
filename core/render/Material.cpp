#include "Material.h"
#include "render/util.h"

namespace core::render {

Material::Material(Device* device,
                   AssetView<ShaderAsset> shaderView,
                   wgpu::Buffer unifromBuffer,
                   std::vector<std::byte> cpuData,
                   std::unordered_map<PropertyId, VariableInfo> variableInfo)
    : m_device(device),
      m_shaderView(shaderView),
      m_uniformBuffer(unifromBuffer),
      m_cpuVariableBufferData(cpuData),
      m_variableInfo(variableInfo) {
    wgpu::SamplerDescriptor samplerDesc{
        .label = "Default Sampler",
        .addressModeU = wgpu::AddressMode::Repeat,
        .addressModeV = wgpu::AddressMode::Repeat,
        .addressModeW = wgpu::AddressMode::Repeat,
        .magFilter = wgpu::FilterMode::Linear,
        .minFilter = wgpu::FilterMode::Linear,
        .mipmapFilter = wgpu::MipmapFilterMode::Linear,
    };
    m_sampler = m_device->GetDeivce().CreateSampler(&samplerDesc);

}
void Material::UpdateUniform() {
    m_device->WriteBuffer(m_uniformBuffer, 0, m_cpuVariableBufferData.data(),
                          m_cpuVariableBufferData.size());
    m_isUniformDirty = false;
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
                wgpu::BindGroupEntry samplerEntry{
                    .binding = entryInfo.binding,
                    .sampler = m_sampler,  // TODO: support sampler in material
                };
                entries.push_back(samplerEntry);
                break;
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

wgpu::BindGroup Material::GetBindGroup() {
    if (m_bindGroup == nullptr) {
        RebuildBindGroup();
    }
    return m_bindGroup;
}

void Material::SetTexture(const std::string& name, AssetView<Texture> texture) {
    SetTexture(ToPropertyID(name), texture);
}

void Material::SetTexture(PropertyId id, AssetView<Texture> texture) {
    m_textures[id] = texture;
    m_bindGroup = nullptr;
}

}  // namespace core::render
