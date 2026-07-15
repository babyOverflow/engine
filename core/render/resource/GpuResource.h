#pragma once
#include <dawn/webgpu_cpp.h>
#include <vector>

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

using GpuBuffer = GpuResource<wgpu::Buffer>;
using GpuPipelineLayout = GpuResource<wgpu::PipelineLayout>;
using GpuBindGroupLayout = GpuResource<wgpu::BindGroupLayout>;
using GpuBindGroup = GpuResource<wgpu::BindGroup>;
using GpuRenderPipeline = GpuResource<wgpu::RenderPipeline>;
}  // namespace core::render