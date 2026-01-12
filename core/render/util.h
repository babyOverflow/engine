#pragma once
#include <print>
#include <windows.h>

// #if DAWN_PLATFORM_IS(WINDOWS)
#define GLFW_EXPOSE_NATIVE_WIN32
// #endif
#if defined(DAWN_USE_X11)
#define GLFW_EXPOSE_NATIVE_X11
#endif
#if defined(DAWN_USE_WAYLAND)
#define GLFW_EXPOSE_NATIVE_WAYLAND
#endif

#include <libloaderapi.h>
#include <GLFW/glfw3native.h>
#include "Window.h"
#include <dawn/webgpu_cpp.h>


namespace core::util {
std::unique_ptr<wgpu::ChainedStruct, void (*)(wgpu::ChainedStruct*)>
SetupWindowAndGetSurfaceDescriptor(GLFWwindow* window) {
    if (glfwGetWindowAttrib(window, GLFW_CLIENT_API) != GLFW_NO_API) {
        // dawn::ErrorLog() << "GL context was created on the window. Disable context creation by "
        //     "setting the GLFW_CLIENT_API hint to GLFW_NO_API.";
        return {nullptr, [](wgpu::ChainedStruct*) {}};
    }
    // #if DAWN_PLATFORM_IS(WINDOWS)
    wgpu::SurfaceSourceWindowsHWND* desc = new wgpu::SurfaceSourceWindowsHWND();
    desc->hwnd = glfwGetWin32Window(window);
    desc->hinstance = GetModuleHandle(nullptr);
    if (desc->hwnd == nullptr)
    {
        std::println("failt to get hwnd: {}, ", GetLastError());
    }
    return {desc, [](wgpu::ChainedStruct* desc) {
                delete reinterpret_cast<wgpu::SurfaceSourceWindowsHWND*>(desc);
            }};
    // #elif defined(DAWN_ENABLE_BACKEND_METAL)
    //         return SetupWindowAndGetSurfaceDescriptorCocoa(window);
    // #elif defined(DAWN_USE_WAYLAND) || defined(DAWN_USE_X11)
    // #if defined(GLFW_PLATFORM_WAYLAND) && defined(DAWN_USE_WAYLAND)
    //         if (glfwGetPlatform() == GLFW_PLATFORM_WAYLAND) {
    //             wgpu::SurfaceSourceWaylandSurface* desc = new
    //             wgpu::SurfaceSourceWaylandSurface(); desc->display = glfwGetWaylandDisplay();
    //             desc->surface = glfwGetWaylandWindow(window);
    //             return { desc, [](wgpu::ChainedStruct* desc) {
    //                         delete reinterpret_cast<wgpu::SurfaceSourceWaylandSurface*>(desc);
    //                     } };
    //         }
    //         else  // NOLINT(readability/braces)
    // #endif
    // #if defined(DAWN_USE_X11)
    //         {
    //             wgpu::SurfaceSourceXlibWindow* desc = new wgpu::SurfaceSourceXlibWindow();
    //             desc->display = glfwGetX11Display();
    //             desc->window = glfwGetX11Window(window);
    //             return { desc, [](wgpu::ChainedStruct* desc) {
    //                         delete reinterpret_cast<wgpu::SurfaceSourceXlibWindow*>(desc);
    //                     } };
    //         }
    // #else
    //         {
    //             return { nullptr, [](wgpu::ChainedStruct*) {} };
    //         }
    // #endif
    // #else
    // return { nullptr, [](wgpu::ChainedStruct*) {} };
    // #endif
}

wgpu::Surface CreateSurfaceForWGPU(wgpu::Instance instance, Window& window) {
    auto* glfwWin = window.GetGLFWwindow();
    if (glfwWin == nullptr) {
        std::println("FATAL: Provided GLFWwindow is null!");
        return nullptr;
    }
    auto chainedDescriptor = SetupWindowAndGetSurfaceDescriptor(glfwWin);
    wgpu::SurfaceDescriptor descriptor;
    descriptor.nextInChain = chainedDescriptor.get();
    wgpu::Surface surface = instance.CreateSurface(&descriptor);
    return surface;
}

using wgpu::TextureFormat;
bool IsSRGB(wgpu::TextureFormat format) {
    switch (format) {
        case TextureFormat::BC1RGBAUnormSrgb:
        case TextureFormat::BC2RGBAUnormSrgb:
        case TextureFormat::BC3RGBAUnormSrgb:
        case TextureFormat::BC7RGBAUnormSrgb:
        case TextureFormat::BGRA8UnormSrgb:
        case TextureFormat::RGBA8UnormSrgb:
        case TextureFormat::ETC2RGB8A1UnormSrgb:
        case TextureFormat::ETC2RGB8UnormSrgb:
            return true;
        default:
            return false;
    }
}
wgpu::TextureFormat SelectSurfaceFormat(const wgpu::Adapter& adapter,
                                        const wgpu::Surface& surface) {
    wgpu::SurfaceCapabilities capabilities;
    wgpu::ConvertibleStatus status = surface.GetCapabilities(adapter, &capabilities);

    wgpu::TextureFormat surface_format = capabilities.formats[0];
    for (int i = 0; i < capabilities.formatCount; i++) {
        if (IsSRGB(capabilities.formats[i])) {
            surface_format = capabilities.formats[i];
        }
    }
    return surface_format;
}
}  // namespace core::util