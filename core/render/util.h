#pragma once
#include <windows.h>
#include <print>

// #if DAWN_PLATFORM_IS(WINDOWS)
#define GLFW_EXPOSE_NATIVE_WIN32
// #endif
#if defined(DAWN_USE_X11)
#define GLFW_EXPOSE_NATIVE_X11
#endif
#if defined(DAWN_USE_WAYLAND)
#define GLFW_EXPOSE_NATIVE_WAYLAND
#endif

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <dawn/webgpu_cpp.h>
#include <libloaderapi.h>
#include "ShaderAssetFormat.h"
#include "Window.h"

namespace core::util {
std::unique_ptr<wgpu::ChainedStruct, void (*)(wgpu::ChainedStruct*)>
SetupWindowAndGetSurfaceDescriptor(GLFWwindow* window);

wgpu::Surface CreateSurfaceForWGPU(wgpu::Instance instance, Window& window);
bool IsSRGB(wgpu::TextureFormat format);
wgpu::TextureFormat SelectSurfaceFormat(const wgpu::Adapter& adapter, const wgpu::Surface& surface);

struct WgpuShaderBindingLayoutInfo {
    std::vector<wgpu::BindGroupLayoutEntry> entries;

    struct GroupRange {
        uint32_t offset = 0;
        uint32_t count = 0;  // count가 0이면 해당 Set은 없는 것
    };

    // WebGPU 표준 제한(MaxBindGroups)은 보통 4입니다.
    // 인덱스가 곧 Set 번호가 됩니다.
    std::array<GroupRange, 4> groups;

    const std::span<const wgpu::BindGroupLayoutEntry> GetGroup(uint32_t setIdx) const;

    static WgpuShaderBindingLayoutInfo MergeVisibility(const WgpuShaderBindingLayoutInfo& a,
                                                       const WgpuShaderBindingLayoutInfo& b);
};

WgpuShaderBindingLayoutInfo MapShdrBindToWgpu(std::span<const ShaderAsset::Binding> shdrBinding);
}  // namespace core::util