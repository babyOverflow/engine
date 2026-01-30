#include "MaterialSystem.h"

namespace core::render {

MaterialSystem::MaterialSystem(Device* device,
                               LayoutCache* layoutCache,
                               PipelineManager* piplineManager)
    : m_device(device), m_layoutCache(layoutCache), m_pipelineManager(piplineManager) {}

Material MaterialSystem::CreateMaterialFromShader(AssetView<ShaderAsset> shaderAsset) {
    if (!shaderAsset->IsValidRenderShader()) {
        return Material();
    }

    const ShaderReflectionData& bindGroupInfos = shaderAsset->GetReflection();

    const auto variables = bindGroupInfos.GetMaterialVariableInfos();

    size_t bufferSize = bindGroupInfos.materialUniformSize;
    if (bufferSize) {
        std::unordered_map<PropertyId, Material::VariableInfo> variableInfo;
        for (const auto& var : variables) {
            variableInfo[var.id] = {
                .offset = var.offset,
                .size = var.size,
            };
        }

        wgpu::BufferDescriptor bufferDesc{
            .usage = wgpu::BufferUsage::Uniform | wgpu::BufferUsage::CopyDst,
            .size = bufferSize,
        };
        wgpu::Buffer buffer = m_device->CreateBuffer(bufferDesc);
        std::vector<std::byte> emptyData(bufferSize, std::byte{0});
        return Material(m_device, shaderAsset, std::move(buffer), std::move(emptyData),
                        std::move(variableInfo));
    }
    std::vector<uint8_t> pinkTextureData = {255, 255, 255, 255};
    core::memory::StridedSpan<const uint8_t> pinkTextureSpan(pinkTextureData.data(), 1, 1);
    const wgpu::TextureDescriptor desc{};
    auto pinkTexture = m_device->CreateTextureFromData<uint8_t>(desc, pinkTextureSpan);
    return Material(m_device, shaderAsset);
}

}