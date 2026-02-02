#include "TextureManager.h"
#include "TextureAssetFormat.h"
#include "util.h"

namespace core::render {
Handle TextureManager::LoadTexture(const TextureAssetFormat& assetData) {
    wgpu::TextureDescriptor desc{
        .usage = wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::TextureBinding,
        .dimension = util::ConvertTextureDimensionWgpu(assetData.dimension),
        .size = {assetData.width, assetData.height, assetData.depth},
        .format = util::ConvertTextureFormatWgpu(assetData.format),
        .mipLevelCount = assetData.mips,
    };

    memory::StridedSpan<const uint8_t> data(assetData.pixelData.data(), assetData.channel,
                                            assetData.pixelData.size());

    wgpu::Texture wgpuTexture = m_device->CreateTextureFromData(desc, data);
    render::Texture texture(wgpuTexture);
    texture.CreateDefaultView(nullptr);
    texture.SetDesc(desc.usage, desc.dimension, desc.format, desc.size, assetData.mips);
    return m_assetRepo->StoreTexture(std::move(texture));
}
}  // namespace core::render