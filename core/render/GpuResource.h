#pragma once
#include <dawn/webgpu_cpp.h>
#include <vector>
#include "render/util.h"

namespace core::render {

template <typename WGPUEntity>
class GpuResource {
  public:
    GpuResource() = default;
    GpuResource(WGPUEntity entity) : handle_(entity) {}
    ~GpuResource() = default;

    GpuResource(const GpuResource&) = delete;
    GpuResource& operator=(const GpuResource&) = delete;

    GpuResource(GpuResource&& other) noexcept : handle_(other.handle_) { other.handle_ = nullptr; }
    GpuResource& operator=(GpuResource&& other) noexcept {
        if (this != &other) {
            // handle_.Release();
            handle_ = other.handle_;
            other.handle_ = nullptr;
        }
        return *this;
    }

    const WGPUEntity& GetHandle() const { return handle_; }
    WGPUEntity* operator->() { return &handle_; }

  private:
    WGPUEntity handle_;
};

class GpuShaderModule : public GpuResource<wgpu::ShaderModule> {
  public:
    using GpuResource<wgpu::ShaderModule>::GpuResource;
    GpuShaderModule() = default;
    const core::util::WgpuShaderBindingLayoutInfo& GetBindGroupEntries() { return m_entries; };
    [[deprecated(
        "Temporary method don't use it. BindGrpupEntries should be created when creating shader "
        "module")]]
    void SetBindGroupEntries(core::util::WgpuShaderBindingLayoutInfo entries) {
        m_entries = entries;
    };

  private:
    core::util::WgpuShaderBindingLayoutInfo m_entries;
};

class GpuTexture : public GpuResource<wgpu::Texture> {
  public:
    using GpuResource<wgpu::Texture>::GpuResource;
    GpuTexture() = default;
    void SetDesc(wgpu::TextureUsage usage,
                 wgpu::TextureDimension dimension,
                 wgpu::TextureFormat format,
                 wgpu::Extent3D size) {
        m_usage = usage;
        m_dimension = dimension;
        m_format = format;
        m_size = size;
    }
    wgpu::TextureUsage GetUsage() const { return m_usage; }
    wgpu::TextureDimension GetDimension() const { return m_dimension; }
    wgpu::TextureFormat GetFormat() const { return m_format; }
    wgpu::Extent3D GetSize() const { return m_size; }
    void CreateDefaultView(const wgpu::TextureViewDescriptor* descriptor) {
        m_textureView = this->GetHandle().CreateView(descriptor);
    }
    wgpu::TextureView GetView() { return m_textureView; }

  private:
    wgpu::TextureUsage m_usage = wgpu::TextureUsage::None;
    wgpu::TextureDimension m_dimension = wgpu::TextureDimension::e2D;
    wgpu::TextureFormat m_format = wgpu::TextureFormat::Undefined;
    wgpu::Extent3D m_size = {0, 0, 0};
    wgpu::TextureView m_textureView;
};

using GpuBuffer = GpuResource<wgpu::Buffer>;
using GpuPipelineLayout = GpuResource<wgpu::PipelineLayout>;
using GpuBindGroupLayout = GpuResource<wgpu::BindGroupLayout>;
using GpuBindGroup = GpuResource<wgpu::BindGroup>;
using GpuRenderPipeline = GpuResource<wgpu::RenderPipeline>;
}  // namespace core::render