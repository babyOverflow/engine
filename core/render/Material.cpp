#include "IRenderPass.h"
#include "Material.h"
#include "render/util.h"

namespace core::render {

Material::Material() : m_device(nullptr), m_shaderView({}) {}

Material::Material(Material&& rhs) noexcept
    : m_device(rhs.m_device),
      m_passManager(rhs.m_passManager),
      m_shaderView(std::move(rhs.m_shaderView)),
      m_bindGroups(std::move(rhs.m_bindGroups)),
      m_sampler(std::move(rhs.m_sampler)),
      m_uniformBuffer(std::move(rhs.m_uniformBuffer)),
      m_cpuVariableBufferData(std::move(rhs.m_cpuVariableBufferData)),
      m_variableInfo(std::move(rhs.m_variableInfo)),
      m_textures(std::move(rhs.m_textures)),
      m_activePassIds(std::move(rhs.m_activePassIds)) {
    rhs.m_device = nullptr;
}

Material& Material::operator=(Material&& rhs) noexcept {
    if (this != &rhs) {
        m_device = rhs.m_device;
        m_passManager = rhs.m_passManager;
        m_shaderView = std::move(rhs.m_shaderView);
        m_bindGroups = std::move(rhs.m_bindGroups);
        m_sampler = std::move(rhs.m_sampler);
        m_uniformBuffer = std::move(rhs.m_uniformBuffer);
        m_cpuVariableBufferData = std::move(rhs.m_cpuVariableBufferData);
        m_variableInfo = std::move(rhs.m_variableInfo);
        m_textures = std::move(rhs.m_textures);
        m_activePassIds = std::move(rhs.m_activePassIds);

        rhs.m_device = nullptr;
    }
    return *this;
}

Material::Material(Device* device,
                   PassManager* passManager,
                   AssetView<ShaderAsset> shaderView,
                   wgpu::Buffer uniformBuffer,
                   std::vector<std::byte> cpuData,
                   std::unordered_map<PropertyId, VariableInfo> variableInfo)
    : m_device(device),
      m_passManager(passManager),
      m_shaderView(shaderView),
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
    return !m_bindGroupDirtyFlags.none();
}

wgpu::BindGroup Material::CreateBindGroupForPass(uint32_t passId) {
    const ShaderReflection& reflection = m_shaderView->GetReflection();
    const uint32_t shaderEntryCount = reflection.GetEntryPointCount();

    const auto pass = m_passManager->CreatePass(passId);
    const auto entryOpt = reflection.GetEntryPointOffsetByName(pass->GetVertexEntryName());

    if (!entryOpt.has_value()) {
        return nullptr;
    }

    uint32_t shaderEntry = entryOpt.value();
    const wgpu::BindGroupLayout bindGroupLayout =
        m_shaderView->GetBindGroupLayout(shaderEntry, BindSlot::Material);
    std::span<const ShaderReflection::Binding> bindings =
        reflection.GetGroup(shaderEntry, BindSlot::Material);

    std::vector<wgpu::BindGroupEntry> entries;
    for (uint32_t i = 0; i < bindings.size(); ++i) {
        const auto& entryInfo = bindings[i];
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
                    .sampler = m_sampler,
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
    return m_device->CreateBindGroup(bindGroupDesc);
}

void Material::RebuildBindGroup() {
    const ShaderReflection& reflection = m_shaderView->GetReflection();
    const uint32_t shaderEntryCount = reflection.GetEntryPointCount();

    for (auto id : m_activePassIds) {
        m_bindGroups[id] = CreateBindGroupForPass(id);
    }
}

wgpu::BindGroup Material::GetBindGroup(uint32_t passId) const {
    return m_bindGroups[passId];
}

void Material::SetTexture(const std::string& name, AssetView<Texture> texture) {
    SetTexture(ToPropertyID(name), texture);
}

void Material::SetTexture(PropertyId id, AssetView<Texture> texture) {
    m_textures[id] = texture;
}

void Material::FlushDirtyBindGroups() {
    if (IsDirty()) {
        RebuildBindGroup();
        m_bindGroupDirtyFlags.reset();
    }
}
}  // namespace core::render
