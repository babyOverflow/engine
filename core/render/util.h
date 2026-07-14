#pragma once
#include <windows.h>

// #if DAWN_PLATFORM_IS(WINDOWS)
#define GLFW_EXPOSE_NATIVE_WIN32
// #endif
#ifdef DAWN_USE_X11
#define GLFW_EXPOSE_NATIVE_X11
#endif
#ifdef DAWN_USE_WAYLAND
#define GLFW_EXPOSE_NATIVE_WAYLAND
#endif

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include <dawn/webgpu_cpp.h>
#include <libloaderapi.h>
#include "TextureAssetFormat.h"
#include "Window.h"
#include "render/resource/ShaderAsset.h"

namespace core::util {
std::unique_ptr<wgpu::ChainedStruct, void (*)(wgpu::ChainedStruct*)>
SetupWindowAndGetSurfaceDescriptor(GLFWwindow* window);

wgpu::Surface CreateSurfaceForWGPU(wgpu::Instance instance, Window& window);
bool IsSRGB(wgpu::TextureFormat format);
wgpu::TextureFormat SelectSurfaceFormat(const wgpu::Adapter& adapter, const wgpu::Surface& surface);

wgpu::BindGroupLayoutEntry MapBindingInfoToWgpu(core::render::ShaderReflection::Binding binding);

wgpu::TextureFormat ConvertTextureFormatWgpu(core::TextureFormat format);
wgpu::TextureDimension ConvertTextureDimensionWgpu(core::TextureDimension dimension);

}  // namespace core::util
