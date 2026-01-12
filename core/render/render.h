// render.h: 표준 시스템 포함 파일
// 또는 프로젝트 특정 포함 파일이 들어 있는 포함 파일입니다.

#pragma once

#include <dawn/webgpu_cpp.h>
#include <glm/glm.hpp>
#include <iostream>
#include <memory>

#include "wgx.h"
#include "GpuResource.h"
#include "Window.h"
#include "ShaderInterop.h"

namespace core {
namespace render {

struct CameraUniformData {
    alignas(16) glm::mat4x4 view;
    alignas(16) glm::mat4x4 proj;
    alignas(16) glm::mat4x4 viewProj;
    alignas(16) glm::vec3 position;
};

class Device {
  public:
    Device() = delete;
    static std::unique_ptr<Device> Create(Window& window);
    ~Device() = default;

    void Present();

    const wgpu::Device& GetDeivce() { return m_device; }
    const wgpu::SurfaceConfiguration& GetSurfaceConfig() { return m_surfaceConfig; }
    wgpu::TextureView GetCurrentTextureView();

    GpuShaderModule CreateShaderModuleFromWGSL(const std::string_view wgslCode);
    GpuShaderModule CreateShaderModuleFromSPIRV(const std::vector<uint32_t>& spirvCode);

    GpuBuffer CreateBufferFromData(const void* data, size_t size, wgpu::BufferUsage usage);

    GpuBindGroupLayout CreateBindGroupLayout(const wgpu::BindGroupLayoutDescriptor& descriptor);
    GpuBindGroup CreateBindGroup(const wgpu::BindGroupDescriptor& descriptor);

    GpuPipelineLayout CreatePipelineLayout(const wgpu::PipelineLayoutDescriptor& descriptor);
    GpuRenderPipeline CreateRenderPipeline(const wgpu::RenderPipelineDescriptor& descriptor);

    void WriteBuffer(const GpuBuffer& buffer, uint64_t offset, void* data, uint64_t size);

  private:
    Device(wgpu::Instance instance,
           wgpu::Adapter adapter,
           wgpu::Device device,
           wgpu::Surface surface,
           wgpu::SurfaceConfiguration surfaceConfig)
        : m_instance(instance),
          m_adapter(adapter),
          m_device(device),
          m_surface(surface),
          m_surfaceConfig(surfaceConfig) {}

    wgpu::Instance m_instance;
    wgpu::Adapter m_adapter;
    wgpu::Device m_device;
    wgpu::Surface m_surface;
    wgpu::SurfaceConfiguration m_surfaceConfig;
};

bool IsSRGB(wgpu::TextureFormat format);



wgpu::Buffer CreateBufferFromData(wgpu::Device& device,
                                  const void* data,
                                  size_t size,
                                  wgpu::BufferUsage usage);

std::expected<std::string, int> LoadShaderCode(std::string_view path);

std::expected<wgpu::ShaderModule, int> LoadShaderModuleFromString(wgpu::Device& device,
                                                                  std::string_view shaderCode);


}  // namespace render

}  // namespace core