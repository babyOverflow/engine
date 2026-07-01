#pragma once
#include <Common.h>
#include <webgpu/webgpu_cpp.h>
#include <string>

namespace core::render {

class Texture {
  public:
    inline static AssetPath kDefaultTexture = {"__default_texture__"};

    Texture() = default;
    Texture(wgpu::Texture wgpuHandle) : m_texture(wgpuHandle) {}

    Texture& operator=(const Texture& other) = delete;
    Texture(Texture& other) = delete;

    Texture& operator=(Texture&& other) noexcept = default;
    Texture(Texture&& other) noexcept = default;

    void SetDesc(wgpu::TextureUsage usage,
                 wgpu::TextureDimension dimension,
                 wgpu::TextureFormat format,
                 wgpu::Extent3D size,
                 uint32_t mipLevelCount) {
        m_usage = usage;
        m_dimension = dimension;
        m_format = format;
        m_size = size;
        m_mipLevelCount = mipLevelCount;
    }

    void CreateDefaultView(const wgpu::TextureViewDescriptor* descriptor = nullptr) {
        m_textureView = m_texture.CreateView(descriptor);
    }

    wgpu::TextureUsage GetUsage() const { return m_usage; }
    wgpu::TextureDimension GetDimension() const { return m_dimension; }
    wgpu::TextureFormat GetFormat() const { return m_format; }
    wgpu::Extent3D GetSize() const { return m_size; }
    wgpu::TextureView GetView() { return m_textureView; }
    uint32_t GetMipLevel() { return m_mipLevelCount; }

  private:
    wgpu::Texture m_texture;
    wgpu::TextureUsage m_usage;
    wgpu::TextureDimension m_dimension;
    wgpu::TextureFormat m_format;
    wgpu::Extent3D m_size;
    wgpu::TextureView m_textureView;
    uint32_t m_mipLevelCount = 1;
};
}  // namespace core::render