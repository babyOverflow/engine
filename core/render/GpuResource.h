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
    [[deprecated("Temporary method don't use it. BindGrpupEntries should be created when creating shader module")]]
    void SetBindGroupEntries(core::util::WgpuShaderBindingLayoutInfo entries) {
        m_entries = entries;
    };

  private:
    core::util::WgpuShaderBindingLayoutInfo m_entries;
};

using GpuBuffer = GpuResource<wgpu::Buffer>;
using GpuPipelineLayout = GpuResource<wgpu::PipelineLayout>;
using GpuBindGroupLayout = GpuResource<wgpu::BindGroupLayout>;
using GpuBindGroup = GpuResource<wgpu::BindGroup>;
using GpuRenderPipeline = GpuResource<wgpu::RenderPipeline>;
}  // namespace core::render