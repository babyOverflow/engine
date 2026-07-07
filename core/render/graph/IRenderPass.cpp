#include "IRenderPass.h"

void core::render::BlackBoard::Set(const std::string& key, Handle value) {
    PropertyId id = ToPropertyID(key);
    m_data[id] = value;
}
void core::render::BlackBoard::Set(const PropertyId key, Handle value) {
    m_data[key] = value;
}

core::Handle core::render::BlackBoard::Get(const std::string& key) const {
    return m_data.at(ToPropertyID(key));
}
core::Handle core::render::BlackBoard::Get(const PropertyId key) const {
    return m_data.at(key);
}

core::Handle core::render::PassSetupContext::DeclareTexture(const std::string& bindingName,
                                                            const TextureDescriptor& desc) {
    LocalTexture texture{.name = bindingName, .textureDesc = desc};

    m_declaredTextures.push_back(texture);
    uint32_t index = m_requiredTextures.size();
    m_requiredTextures.push_back(texture);
    return Handle{.index = index, .generation = 0};
}

void core::render::PassSetupContext::RegisterPassOutputs(
    std::vector<ColorAttachment> colorAttachments,
    std::optional<DepthStencilAttachment> depthStencilAttachment) {
    m_colorAttachments = std::move(colorAttachments);
    m_depthStencilAttachment = std::move(depthStencilAttachment);
}

void core::render::PassSetupContext::RegisterTextureRead(Handle texture) {
    m_readTextures.push_back({texture});
}
void core::render::PassSetupContext::RegisterTextureRead(Handle texture,
                                                         wgpu::TextureViewDescriptor viewDesc) {
    m_readTextures.push_back({texture, viewDesc});
}

core::Handle core::render::PassSetupContext::GetResourceHandle(const std::string& name) {
    uint32_t index = m_requiredTextures.size();
    LocalTexture texture{.name = name};
    m_requiredTextures.push_back(texture);
    return Handle(index);
}


core::render::IRenderPass* core::render::PassManager::GetPass(uint8_t id) const {
    return m_passes[id].get();
}

uint8_t core::render::PassManager::GetPassID(const std::string_view passName) const {
    auto it = m_nameToId.find(passName);
    if (it == m_nameToId.end()) {
        assert(false && "Pass ID not found");
    }
    return it->second;
}
