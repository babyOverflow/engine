#include "IRenderPass.h"
#include "Material.h"
#include "render/util.h"

namespace core::render {

Material::Material() : m_device(nullptr) {}

Material::Material(Material&& rhs) noexcept
    : m_device(rhs.m_device),
      m_sampler(std::move(rhs.m_sampler)),
      m_uniformBuffer(std::move(rhs.m_uniformBuffer)),
      m_cpuVariableBufferData(std::move(rhs.m_cpuVariableBufferData)),
      m_variableInfo(std::move(rhs.m_variableInfo)),
      m_textures(std::move(rhs.m_textures)),
      m_activeTechniqueId(std::move(rhs.m_activeTechniqueId)) {
    rhs.m_device = nullptr;
}

Material& Material::operator=(Material&& rhs) noexcept {
    if (this != &rhs) {
        m_device = rhs.m_device;
        m_sampler = std::move(rhs.m_sampler);
        m_uniformBuffer = std::move(rhs.m_uniformBuffer);
        m_cpuVariableBufferData = std::move(rhs.m_cpuVariableBufferData);
        m_variableInfo = std::move(rhs.m_variableInfo);
        m_textures = std::move(rhs.m_textures);
        m_activeTechniqueId = std::move(rhs.m_activeTechniqueId);

        rhs.m_device = nullptr;
    }
    return *this;
}

Material::Material(Device* device,
                   wgpu::Buffer uniformBuffer,
                   std::vector<std::byte> cpuData,
                   std::unordered_map<PropertyId, VariableInfo> variableInfo)
    : m_device(device),
      m_uniformBuffer(uniformBuffer),
      m_cpuVariableBufferData(std::move(cpuData)),
      m_variableInfo(std::move(variableInfo)) {
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
}

bool Material::IsDirty() const {
    return m_isDirty;
}

//wgpu::BindGroup Material::CreateBindGroupForPass(uint32_t passId) {
//    const ShaderReflection& reflection = m_shaderView->GetReflection();
//
//    const wgpu::BindGroupLayout bindGroupLayout =
//        m_shaderView->GetBindGroupLayout(BindSlot::Material);
//    std::span<const ShaderReflection::Binding> bindings =
//        reflection.GetGroup(BindSlot::Material);
//
    //std::vector<wgpu::BindGroupEntry> bindGroupEntries;
    //for (uint32_t i = 0; i < bindings.size(); ++i) {
    //    const ShaderReflection::Binding& bindingInfo = bindings[i];
    //    switch (bindingInfo.resourceType) {
    //        case core::ShaderAssetFormat::ResourceType::UniformBuffer: {
    //            wgpu::BindGroupEntry bufferEntry{
    //                .binding = bindingInfo.binding,
    //                .buffer = m_uniformBuffer,
    //                .offset = 0,
    //                .size = reflection.materialUniformSize,
    //            };
    //            bindGroupEntries.push_back(bufferEntry);
    //            break;
    //        }
    //        case core::ShaderAssetFormat::ResourceType::Texture: {
    //            auto it = m_textures.find(bindingInfo.id);
    //            if (it == m_textures.end()) {
    //                it = m_textures.find(ToPropertyID(Texture::kDefaultTexture.value));
    //            }
    //            const auto& texture = it->second;
    //            wgpu::BindGroupEntry textureEntry{
    //                .binding = bindingInfo.binding,
    //                .textureView = texture->GetView(),
    //            };
    //            bindGroupEntries.push_back(textureEntry);
    //            break;
    //        }
    //        case core::ShaderAssetFormat::ResourceType::Sampler: {
    //            wgpu::BindGroupEntry samplerEntry{
    //                .binding = bindingInfo.binding,
    //                .sampler = m_sampler,
    //            };
    //            bindGroupEntries.push_back(samplerEntry);
    //            break;
    //        }
    //        default:
    //            assert(false && "not supported resource type.");

    //            break;
    //    }
    //}

    //wgpu::BindGroupDescriptor bindGroupDesc{
    //    .layout = bindGroupLayout,
    //    .entryCount = static_cast<uint32_t>(bindGroupEntries.size()),
    //    .entries = bindGroupEntries.data(),
    //};
//    return m_device->CreateBindGroup(bindGroupDesc);
//}

//void Material::RebuildBindGroup() {
//    const ShaderReflection& reflection = m_shaderView->GetReflection();
//
//    for (auto id : m_activePassIds) {
//        m_bindGroups[id] = CreateBindGroupForPass(id);
//    }
//}


void Material::SetTexture(const std::string& name, AssetView<Texture> texture) {
    SetTexture(ToPropertyID(name), texture);
}

void Material::SetTexture(PropertyId id, AssetView<Texture> texture) {
    m_textures[id] = texture;
}

AssetView<Texture> Material::GetTexture(PropertyId id) {
    return m_textures[id];
}

}  // namespace core::render
