// render.cpp : 애플리케이션의 진입점을 정의합니다.
//
#include "render.h"
#include <dawn/webgpu_cpp.h>
#include <magic_enum/magic_enum.hpp>
#include <print>
#include "util.h"

namespace core {
namespace render {

std::unique_ptr<Device> core::render::Device::Create(Window& window) {
    const auto features = wgpu::InstanceFeatureName::TimedWaitAny;
    wgpu::InstanceDescriptor descriptor = {.requiredFeatureCount = 1,
                                           .requiredFeatures = &features};
    wgpu::Instance instance = wgpu::CreateInstance(&descriptor);

    wgpu::Adapter adapter;
    const wgpu::RequestAdapterOptions option{
        .powerPreference = wgpu::PowerPreference::Undefined,
        .forceFallbackAdapter = false,
    };
    wgpu::Future f1 = instance.RequestAdapter(
        &option, wgpu::CallbackMode::WaitAnyOnly,
        [&](wgpu::RequestAdapterStatus status, wgpu::Adapter a, wgpu::StringView message) {
            if (status != wgpu::RequestAdapterStatus::Success) {
                std::println("RequestAdapter: {}", message.data);
                exit(0);
            }
            adapter = std::move(a);
        });
    instance.WaitAny(f1, UINT64_MAX);

    wgpu::DeviceDescriptor deviceDescriptor{

    };

    wgpu::Device device;
    deviceDescriptor.SetUncapturedErrorCallback([](const wgpu::Device&, wgpu::ErrorType errorType,
                                                   wgpu::StringView message) {
        std::println("UncapturedError: {} - {}", message.data, magic_enum::enum_name(errorType));
    });
    wgpu::Future f2 = adapter.RequestDevice(
        &deviceDescriptor, wgpu::CallbackMode::WaitAnyOnly,
        [&](wgpu::RequestDeviceStatus status, wgpu::Device d, wgpu::StringView message) {
            if (status != wgpu::RequestDeviceStatus::Success) {
                std::println("RequestDevice: {}", message.data);
                exit(0);
            }
            device = std::move(d);
        });
    instance.WaitAny(f2, UINT64_MAX);

    wgpu::Surface surface = core::util::CreateSurfaceForWGPU(instance, window);
    wgpu::SurfaceCapabilities capabilities;
    surface.GetCapabilities(adapter, &capabilities);
    wgpu::SurfaceConfiguration config{
        .device = device,
        .format = capabilities.formats[0],
        .usage = wgpu::TextureUsage::RenderAttachment,
        .width = window.GetWidth(),
        .height = window.GetHeight(),
        .presentMode = wgpu::PresentMode::Fifo,
    };
    surface.Configure(&config);

    return std::unique_ptr<Device>(new Device(instance, adapter, device, surface, config));
}

wgpu::ShaderModule Device::CreateShaderModuleFromWGSL(const std::string_view wgslCode) {
    wgpu::ShaderSourceWGSL wgslSource{{.code = wgpu::StringView(wgslCode)}};
    wgpu::ShaderModuleDescriptor descriptor{
        .nextInChain = &wgslSource,
    };

    wgpu::ShaderModule shaderModule = m_device.CreateShaderModule(&descriptor);
    return shaderModule;
}

wgpu::ShaderModule Device::CreateShaderModuleFromSPIRV(const std::vector<uint32_t>& spirvCode) {
    wgpu::ShaderSourceSPIRV spirvSource;
    spirvSource.codeSize = static_cast<uint32_t>(spirvCode.size());
    spirvSource.code = spirvCode.data();
    wgpu::ShaderModuleDescriptor descriptor{
        .nextInChain = &spirvSource,
    };
    wgpu::ShaderModule shaderModule = m_device.CreateShaderModule(&descriptor);

    return shaderModule;
}

wgpu::Buffer core::render::Device::CreateBufferFromData(const void* data,
                                                        size_t size,
                                                        wgpu::BufferUsage usage) const {
    wgpu::BufferDescriptor bufferDesc{
        .usage = usage | wgpu::BufferUsage::CopyDst,
        .size = size,
        .mappedAtCreation = false,
    };
    wgpu::Buffer buffer = m_device.CreateBuffer(&bufferDesc);
    m_device.GetQueue().WriteBuffer(buffer, 0, data, size);

    return buffer;
}

wgpu::Buffer core::render::Device::CreateBuffer(const wgpu::BufferDescriptor& desc) const {
    return m_device.CreateBuffer(&desc);
}

GpuBindGroupLayout Device::CreateBindGroupLayout(
    const wgpu::BindGroupLayoutDescriptor& descriptor) {
    wgpu::BindGroupLayout layout = m_device.CreateBindGroupLayout(&descriptor);
    return GpuBindGroupLayout(layout);
}

wgpu::BindGroup Device::CreateBindGroup(const wgpu::BindGroupDescriptor& descriptor) {
    return m_device.CreateBindGroup(&descriptor);
}

GpuPipelineLayout Device::CreatePipelineLayout(const wgpu::PipelineLayoutDescriptor& descriptor) {
    wgpu::PipelineLayout layout = m_device.CreatePipelineLayout(&descriptor);
    return GpuPipelineLayout(layout);
}

GpuRenderPipeline Device::CreateRenderPipeline(const wgpu::RenderPipelineDescriptor& descriptor) {
    wgpu::RenderPipeline pipeline = m_device.CreateRenderPipeline(&descriptor);
    return GpuRenderPipeline(pipeline);
}

void Device::WriteBuffer(const GpuBuffer& buffer, uint64_t offset, void* data, uint64_t size) {
    m_device.GetQueue().WriteBuffer(buffer.GetHandle(), offset, data, size);
}

wgpu::Texture Device::CreateTextureFromData(const wgpu::TextureDescriptor& descriptor,
                                            const wgpu::TexelCopyBufferLayout& layout,
                                            std::span<const uint8_t> data) {
    wgpu::Texture texture = m_device.CreateTexture(&descriptor);

    wgpu::TexelCopyTextureInfo destination{
        .texture = texture,
        .mipLevel = 0,
        .origin = {0, 0, 0},
        .aspect = wgpu::TextureAspect::All,
    };

    m_device.GetQueue().WriteTexture(&destination, data.data(), data.size_bytes(), &layout,
                                     &descriptor.size);
    return texture;
}

// template GpuTexture Device::CreateTexture<uint16_t>(const wgpu::TextureDescriptor& desc,
//                                                     core::memory::StridedSpan<const uint16_t>
//                                                     data);
// template GpuTexture Device::CreateTexture<uint32_t>(const wgpu::TextureDescriptor& desc,
//                                                     core::memory::StridedSpan<const uint32_t>
//                                                     data);
// template GpuTexture Device::CreateTexture<float>(const wgpu::TextureDescriptor& desc,
//                                                  core::memory::StridedSpan<const float> data);

void Device::Present() {
    m_surface.Present();
}

wgpu::TextureView Device::GetCurrentTextureView() {
    wgpu::SurfaceTexture surfaceTexture;
    m_surface.GetCurrentTexture(&surfaceTexture);
    return surfaceTexture.texture.CreateView();
}

}  // namespace render
}  // namespace core
