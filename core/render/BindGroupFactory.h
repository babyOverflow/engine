#pragma once
#include <webgpu/webgpu_cpp.h>
#include <concepts>
#include <span>

#include <Common.h>
#include "render.h"
#include "ShaderAsset.h"

namespace core::render {
template <typename T>
concept BindGroupResourceProvider = requires(T provider, PropertyId id) {
    { provider.GetTextureView(id) } -> std::same_as<wgpu::TextureView>;
    // { provider.GetSampler(id) } -> std::same_as<wgpu::Sampler>;
    // { provider.GetBuffer(id) } -> std::same_as<wgpu::Buffer>;
};

class BindGroupFactory {
  public:
    template <typename Provider>
        requires core::render::BindGroupResourceProvider<Provider>
    static wgpu::BindGroup Create(Device* device,
                                  wgpu::BindGroupLayout& layout,
                                  std::span<const ShaderReflection::Binding> bindings,
                                  Provider&& provider);
};
}  // namespace core::render

template <typename Provider>
    requires core::render::BindGroupResourceProvider<Provider>
wgpu::BindGroup core::render::BindGroupFactory::Create(
    Device* device,
    wgpu::BindGroupLayout& layout,
    std::span<const ShaderReflection::Binding> bindings,
    Provider&& provider) {
    std::vector<wgpu::BindGroupEntry> bindGroupEntries;
    bindGroupEntries.reserve(bindings.size());

    for (uint32_t i = 0; i < bindings.size(); ++i) {
        const ShaderReflection::Binding& bindingInfo = bindings[i];
        switch (bindingInfo.resourceType) {
            case core::ShaderAssetFormat::ResourceType::UniformBuffer: {
                assert(false && "Currently uniform variable is not supported");
                wgpu::BindGroupEntry bufferEntry{
                    // TODO!
                };
                bindGroupEntries.push_back(bufferEntry);
                break;
            }
            case core::ShaderAssetFormat::ResourceType::Texture: {
                wgpu::BindGroupEntry textureEntry {
                    .binding = bindingInfo.binding,
                    .textureView = provider.GetTextureView(bindingInfo.id),
                };
                bindGroupEntries.push_back(textureEntry);
                break;
            }
            case core::ShaderAssetFormat::ResourceType::Sampler: {
                wgpu::BindGroupEntry samplerEntry{
                    .binding = bindingInfo.binding,
                    //.sampler = provider.GetSampler(),
                };
                bindGroupEntries.push_back(samplerEntry);
                break;
            }
            default:
                assert(false && "not supported resource type.");

                break;
        }
    }

    wgpu::BindGroupDescriptor bindGroupDesc{
        .layout = layout,
        .entryCount = static_cast<uint32_t>(bindGroupEntries.size()),
        .entries = bindGroupEntries.data(),
    };
    wgpu::BindGroup bindGroup = device->CreateBindGroup(bindGroupDesc);
    return bindGroup;
}
