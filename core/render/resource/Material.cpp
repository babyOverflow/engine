#include "render/graph/IRenderPass.h"
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

void Material::SetTexture(const std::string& name, AssetView<Texture> texture) {
    SetTexture(ToPropertyID(name), texture);
}

void Material::SetTexture(PropertyId id, AssetView<Texture> texture) {
    m_textures[id] = texture;
}

AssetView<Texture> Material::GetTexture(PropertyId id) {
    return m_textures[id];
}

wgpu::TextureView MaterialProvider::GetTextureView(PropertyId id)
{
    return material->GetTexture(id)->GetView();
}

}  // namespace core::render
