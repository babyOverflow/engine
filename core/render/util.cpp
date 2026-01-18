#include "util.h"
#include <map>
#include <ranges>
// TODO!(IsEntryEqual is in LayoutCache.h. It should be moved to proper header. If it's done remove
// inclusion)
#include "LayoutCache.h"

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
    if (desc->hwnd == nullptr) {
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

static wgpu::ShaderStage MapStageToWgpu(ShaderAsset::ShaderVisibility visibility) {
    switch (visibility) {
        case ShaderAsset::ShaderVisibility::Vertex:
            return wgpu::ShaderStage::Vertex;
        case ShaderAsset::ShaderVisibility::Fragment:
            return wgpu::ShaderStage::Fragment;
        case ShaderAsset::ShaderVisibility::Compute:
            return wgpu::ShaderStage::Compute;
        case ShaderAsset::ShaderVisibility::All:
            return wgpu::ShaderStage::Fragment | wgpu::ShaderStage::Vertex;
        case ShaderAsset::ShaderVisibility::None:
            return wgpu::ShaderStage::None;
    }
}

static wgpu::SamplerBindingLayout MapBindingToSampler(const ShaderAsset::Binding& binding) {
    const auto& sampler = binding.resource.sampler;
    switch (sampler.type) {
        case ShaderAsset::SamplerType::Filtering:
            return wgpu::SamplerBindingLayout{
                .type = wgpu::SamplerBindingType::Filtering,
            };
        case ShaderAsset::SamplerType::NonFiltering:
            return wgpu::SamplerBindingLayout{
                .type = wgpu::SamplerBindingType::NonFiltering,
            };
        case ShaderAsset::SamplerType::Comparison:
            return wgpu::SamplerBindingLayout{
                .type = wgpu::SamplerBindingType::Comparison,
            };
        case ShaderAsset::SamplerType::Undefined:
            return wgpu::SamplerBindingLayout{
                .type = wgpu::SamplerBindingType::Undefined,
            };
        case ShaderAsset::SamplerType::BindingNotUsed:
            return wgpu::SamplerBindingLayout{
                .type = wgpu::SamplerBindingType::BindingNotUsed,
            };
        default:
            return {};
    }
}

static wgpu::TextureBindingLayout MapBindingToTexture(const ShaderAsset::Binding& binding) {
    const auto& texture = binding.resource.texture;

    wgpu::TextureBindingLayout layout{};
    switch (texture.type) {
        case ShaderAsset::TextureType::Float:
            layout.sampleType = wgpu::TextureSampleType::Float;
            break;
        case ShaderAsset::TextureType::UnfilterableFloat:
            layout.sampleType = wgpu::TextureSampleType::UnfilterableFloat;
            break;
        case ShaderAsset::TextureType::Depth:
            layout.sampleType = wgpu::TextureSampleType::Depth;
            break;
        case ShaderAsset::TextureType::Sint:
            layout.sampleType = wgpu::TextureSampleType::Sint;
            break;
        case ShaderAsset::TextureType::Uint:
            layout.sampleType = wgpu::TextureSampleType::Uint;
            break;
        case ShaderAsset::TextureType::Undefined:
            layout.sampleType = wgpu::TextureSampleType::Undefined;
            break;
        case ShaderAsset::TextureType::BindingNotUsed:
            layout.sampleType = wgpu::TextureSampleType::BindingNotUsed;
            break;
        default:
            break;
    }

    switch (texture.viewDimension) {
        case ShaderAsset::ViewDimension::e1D:
            layout.viewDimension = wgpu::TextureViewDimension::e1D;
            break;
        case ShaderAsset::ViewDimension::e2D:
            layout.viewDimension = wgpu::TextureViewDimension::e2D;
            break;
        case ShaderAsset::ViewDimension::e2DArray:
            layout.viewDimension = wgpu::TextureViewDimension::e2DArray;
            break;
        case ShaderAsset::ViewDimension::Cube:
            layout.viewDimension = wgpu::TextureViewDimension::Cube;
            break;
        case ShaderAsset::ViewDimension::CubeArray:
            layout.viewDimension = wgpu::TextureViewDimension::CubeArray;
            break;
        case ShaderAsset::ViewDimension::e3D:
            layout.viewDimension = wgpu::TextureViewDimension::e3D;
            break;
        case ShaderAsset::ViewDimension::Undefined:
            layout.viewDimension = wgpu::TextureViewDimension::Undefined;
            break;
        default:
            break;
    }

    if (texture.multiSampled) {
        layout.multisampled = true;
    } else {
        layout.multisampled = false;
    }

    return layout;
}

WgpuShaderBindingLayoutInfo core::util::MapShdrBindToWgpu(
    std::span<const ShaderAsset::Binding> shdrBinding) {
    constexpr uint32_t kMaxBindGroups = 4;

    std::vector<wgpu::BindGroupLayoutEntry> entries;
    entries.reserve(shdrBinding.size());
    std::array<WgpuShaderBindingLayoutInfo::GroupRange, kMaxBindGroups> groups;

    for (uint32_t idx = 0; idx < shdrBinding.size(); ++idx) {
        const auto& binding = shdrBinding[idx];

        // TODO!(replace with DebugAssert)
        assert(entries.empty() || entries.back().binding <= binding.binding ||
               shdrBinding[idx - 1].set < binding.set);

        wgpu::BindGroupLayoutEntry entry{
            .binding = binding.binding,
            .visibility = MapStageToWgpu(binding.visibility),
        };
        switch (binding.resourceType) {
            case ShaderAsset::ResourceType::UniformBuffer:
                entry.buffer = wgpu::BufferBindingLayout{
                    .type = wgpu::BufferBindingType::Uniform,
                    .minBindingSize = binding.resource.buffer.bufferSize,
                };
                break;
            case ShaderAsset::ResourceType::ReadOnlyStorage:
                entry.buffer = wgpu::BufferBindingLayout{
                    .type = wgpu::BufferBindingType::ReadOnlyStorage,
                    .minBindingSize = binding.resource.buffer.bufferSize,
                };
                break;
            case ShaderAsset::ResourceType::StorageBuffer:
                entry.buffer = wgpu::BufferBindingLayout{
                    .type = wgpu::BufferBindingType::Storage,
                    .minBindingSize = binding.resource.buffer.bufferSize,
                };
                break;
            case ShaderAsset::ResourceType::Sampler:
                entry.sampler = MapBindingToSampler(binding);
                break;
            case ShaderAsset::ResourceType::Texture:
                entry.texture = MapBindingToTexture(binding);
                break;
            default:
                break;
        }
        entries.push_back(entry);

        if (groups[binding.set].count == 0) {
            groups[binding.set].offset = entries.size() - 1;
        }
        groups[binding.set].count++;
    }
    return WgpuShaderBindingLayoutInfo{entries, groups};
}

const std::span<const wgpu::BindGroupLayoutEntry> core::util::WgpuShaderBindingLayoutInfo::GetGroup(
    uint32_t setIdx) const {
    if (setIdx >= groups.size() || groups[setIdx].count == 0) {
        return {};
    }
    const auto& range = groups[setIdx];
    return {entries.data() + range.offset, range.count};
}

WgpuShaderBindingLayoutInfo WgpuShaderBindingLayoutInfo::MergeVisibility(
    const WgpuShaderBindingLayoutInfo& a,
    const WgpuShaderBindingLayoutInfo& b) {
    constexpr uint32_t kMaxBindGroups = 4;
    WgpuShaderBindingLayoutInfo info;

    info.entries.reserve(a.entries.size() + b.entries.size());

    for (uint32_t i = 0; i < kMaxBindGroups; ++i) {
        info.groups[i].offset = static_cast<uint32_t>(info.entries.size());

        auto groupA = a.GetGroup(i);
        auto groupB = b.GetGroup(i);

        auto itA = groupA.begin();
        auto itB = groupB.begin();
        const auto endA = groupA.end();
        const auto endB = groupB.end();

        while (itA != endA || itB != endB) {
            if (itA == endA) {
                info.entries.push_back(*itB++);
            } else if (itB == endB) {
                info.entries.push_back(*itA++);
            } else {
                if (itA->binding < itB->binding) {
                    info.entries.push_back(*itA++);
                } else if (itB->binding < itA->binding) {
                    info.entries.push_back(*itB++);
                } else {
                    assert(wgx::IsCompatible(*itA, *itB) &&
                           "Binding Conflict: Same binding index but incompatible types!");

                    wgpu::BindGroupLayoutEntry mergedEntry = *itA;
                    mergedEntry.visibility = itA->visibility | itB->visibility;

                    info.entries.push_back(mergedEntry);
                    itA++;
                    itB++;
                }
            }
        }

        info.groups[i].count = static_cast<uint32_t>(info.entries.size()) - info.groups[i].offset;
    }

    return info;
}

}  // namespace core::util
