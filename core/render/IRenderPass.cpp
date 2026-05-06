#include "IRenderPass.h"

void core::render::BlackBoard::Set(const std::string& key, Handle value) {
    m_data[key] = value;
}

core::Handle core::render::BlackBoard::Get(const std::string& key) const {
    return m_data.at(key);
}

core::render::PassSetupContext::PassSetupContext(Device* device,
                                                 const wgpu::SurfaceConfiguration& surfaceConfig)
    : m_device(device), m_surfaceConfig(surfaceConfig), m_currentPassId(-1) {
    m_subResources.resize(
        1);  // We reserve the first sub resource for scene color, which is a special resource that
             // can be read by multiple passes but only written by one pass.
    m_subResources[0].textureDesc = TextureDescriptor{
        .name = "SceneColor",
        .usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding,
        .dimension = wgpu::TextureDimension::e2D,
        .size = RelativeSize{1.0f, 1.0f},
        .format = surfaceConfig.format,
    };
    m_blackBoard.Set(kSceneColorName, kSceneColorHandle);
}

core::Handle core::render::PassSetupContext::DeclareTexture(const TextureDescriptor& desc) {
    // TODO!(Sunghyun) Temporary replace Relative Size
    const auto* relativeSize = std::get_if<RelativeSize>(&desc.size);
    if (relativeSize != nullptr) {
        TextureDescriptor tempDesc = desc;
        tempDesc.size = wgpu::Extent3D{
            .width = m_surfaceConfig.width * static_cast<int>(relativeSize->widthRatio),
            .height = m_surfaceConfig.height * static_cast<int>(relativeSize->heightRatio)};
        m_subResources.push_back(SubResource{.textureDesc = tempDesc});
    } else {
        m_subResources.push_back(SubResource{.textureDesc = desc});
    }
    return Handle{.index = static_cast<uint32_t>(m_subResources.size() - 1), .generation = 0};
}

void core::render::PassSetupContext::RegisterColorOutput(Handle texture,
                                                         const ColorAttachment& attach) {
    m_subResources[texture.index].writePassInfos.push_back(SubResource::WriteInfo{
        .passId = m_currentPassId,
        .attach = attach,
    });
}
void core::render::PassSetupContext::RegisterDepthStencil(Handle texture,
                                                          const DepthStencilAttachment& attach) {
    m_subResources[texture.index].writePassInfos.push_back(SubResource::WriteInfo{
        .passId = m_currentPassId,
        .attach = attach,
    });
}

void core::render::PassSetupContext::RegisterTextureRead(Handle texture) {
    m_subResources[texture.index].readPassIds.push_back(m_currentPassId);
}

std::unique_ptr<core::render::IRenderPass> core::render::PassManager::CreatePass(uint8_t id) const {
    return m_creators[id]();
}

uint8_t core::render::PassManager::GetPassID(const std::string& passName) const {
    auto it = m_nameToId.find(passName);
    if (it == m_nameToId.end()) {
        assert(false && "Pass ID not found");
    }
    return it->second;
}