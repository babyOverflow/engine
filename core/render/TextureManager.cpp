#include "TextureManager.h"
#include "TextureAssetFormat.h"
#include "util.h"

namespace core::render {
TextureManager::TextureManager(Device* device, AssetManager* assetRepo)
    : m_device(device), m_assetRepo(assetRepo) {
    core::importer::TextureResult defaultTextureResult{
        .textureAsset = kDefaultTextureAsset,
        .assetPath = Texture::kDefaultTexture,
    };
    LoadTexture(defaultTextureResult);
}

Handle TextureManager::LoadTexture(const core::importer::TextureResult& assetResult) {
    if (m_textureCache.find(assetResult.assetPath) != m_textureCache.end()) {
        return m_textureCache[assetResult.assetPath];
    }

    const TextureAssetFormat& assetData = assetResult.textureAsset;

    wgpu::TextureDescriptor desc{
        .usage = wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::TextureBinding,
        .dimension = util::ConvertTextureDimensionWgpu(assetData.dimension),
        .size = {assetData.width, assetData.height, assetData.depth},
        .format = util::ConvertTextureFormatWgpu(assetData.format),
        .mipLevelCount = assetData.mips,
    };

    wgpu::Texture wgpuTexture = m_device->CreateTextureFromData(
        desc,
        wgpu::TexelCopyBufferLayout{
            .bytesPerRow = assetData.channel * assetData.width,
            .rowsPerImage = assetData.height,
        },
        std::span<const uint8_t>((assetData.pixelData.data()), assetData.pixelData.size()));

    render::Texture texture(wgpuTexture);
    texture.CreateDefaultView(nullptr);
    texture.SetDesc(desc.usage, desc.dimension, desc.format, desc.size, assetData.mips);

    Handle handle = m_assetRepo->StoreTexture(std::move(texture));
    m_textureCache[assetResult.assetPath] = handle;
    return handle;
}

AssetView<Texture> TextureManager::GetTexture(const AssetPath& assetPath) {
    if (auto it = m_textureCache.find(assetPath); it != m_textureCache.end()) {
        Handle handle = it->second;
        return m_assetRepo->GetTexture(handle);
    }
    if (auto it = m_textureCache.find(Texture::kDefaultTexture); it != m_textureCache.end()) {
        Handle handle = it->second;
        return m_assetRepo->GetTexture(handle);
    }
    return AssetView<Texture>{};
}
}  // namespace core::render