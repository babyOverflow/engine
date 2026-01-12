#pragma once
#include <dawn/webgpu_cpp.h>

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
            //handle_.Release();
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

  private:

};

using GpuBuffer = GpuResource<wgpu::Buffer>;
using GpuPipelineLayout = GpuResource<wgpu::PipelineLayout>;
using GpuBindGroupLayout = GpuResource<wgpu::BindGroupLayout>;
using GpuBindGroup = GpuResource<wgpu::BindGroup>;
using GpuRenderPipeline = GpuResource<wgpu::RenderPipeline>;
}