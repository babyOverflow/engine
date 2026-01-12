#include "LayoutCache.h"
#include <algorithm>
#include <ranges>

namespace core::render {

BindGroupLayoutKey core::render::BindGroupLayoutKey::From(wgpu::BindGroupLayoutDescriptor& desc) {
    std::span<const wgpu::BindGroupLayoutEntry> entries(desc.entries,
                                                        desc.entries + desc.entryCount);

    // TODO!(rewrite it Debug assertion)
    assert(std::is_sorted(entries.begin(), entries.end(),
                          [](const auto& a, const auto& b) { return a.binding < b.binding; }) &&
           "entries must be sorted");

    return BindGroupLayoutKey{
        .entries = std::ranges::to<std::vector>(entries),
    };
}

wgpu::BindGroupLayout core::render::LayoutCache::GetBindGroupLayout(
    wgpu::BindGroupLayoutDescriptor& desc) {
    auto key = BindGroupLayoutKey::From(desc);

    if (m_bindGroupLayoutCache.contains(key)) {
        return m_bindGroupLayoutCache[key].GetHandle();
    }

    GpuBindGroupLayout bindGroupLayout = m_device->CreateBindGroupLayout(desc);
    m_bindGroupLayoutCache[key] = std::move(bindGroupLayout);
    return m_bindGroupLayoutCache[key].GetHandle();
}

wgpu::PipelineLayout core::render::LayoutCache::GetPipelineLayout(
    std::vector<wgpu::BindGroupLayout> bindGroupLayouts) {
    PipelineLayoutKey key{.layouts = bindGroupLayouts};
    if (m_pipelineLayoutCache.contains(key)) {
        return m_pipelineLayoutCache[key].GetHandle();
    }

    wgpu::PipelineLayoutDescriptor pipelineLayoutDesc{
        .bindGroupLayoutCount = bindGroupLayouts.size(),
        .bindGroupLayouts = bindGroupLayouts.data(),
    };

    GpuPipelineLayout pipelineLayout = m_device->CreatePipelineLayout(pipelineLayoutDesc);
    m_pipelineLayoutCache[key] = std::move(pipelineLayout);
    return m_pipelineLayoutCache[key].GetHandle();
}

bool BindGroupLayoutKey::operator==(const BindGroupLayoutKey& other) const {
    if (entries.size() != other.entries.size()) {
        return false;
    }
    for (uint32_t i = 0; i < entries.size(); ++i) {
        if (!IsEntryEqual(entries[i], other.entries[i])) {
            return false;
        }
    }
    return true;
}

bool IsEntryEqual(const wgpu::BindGroupLayoutEntry& a, const wgpu::BindGroupLayoutEntry& b) {
    if (a.binding != b.binding || a.visibility != b.visibility ||
        a.bindingArraySize != b.bindingArraySize) {
        return false;
    }
    // WebGPU BindGroupLayoutEntry must have only one resource binding.
    if (a.buffer.type != wgpu::BufferBindingType::Undefined) {
        return a.buffer.hasDynamicOffset == b.buffer.hasDynamicOffset &&
               a.buffer.minBindingSize == b.buffer.minBindingSize && a.buffer.type == b.buffer.type;
    } else if (a.texture.sampleType != wgpu::TextureSampleType::Undefined) {
        return a.texture.multisampled == b.texture.multisampled &&
               a.texture.sampleType == b.texture.sampleType &&
               a.texture.viewDimension == b.texture.viewDimension;
    } else if (a.sampler.type != wgpu::SamplerBindingType::Undefined) {
        return a.sampler.type == b.sampler.type;
    } else if (a.storageTexture.access != wgpu::StorageTextureAccess::Undefined) {
        return a.storageTexture.access == b.storageTexture.access &&
               a.storageTexture.format == b.storageTexture.format &&
               a.storageTexture.viewDimension == b.storageTexture.viewDimension;
    }
    return true;
}

bool PipelineLayoutKey::operator==(const PipelineLayoutKey& other) const {
    if (layouts.size() != other.layouts.size()) {
        return false;
    }
    for (uint32_t i = 0; i < layouts.size(); ++i) {
        if (layouts[i].Get() != other.layouts[i].Get()) {
            return false;
        }
    }
    return true;
}

std::size_t PipelineLayoutKeyHash::operator()(const PipelineLayoutKey& key) const {
    std::size_t seed = 0;
    for (const auto& layout : key.layouts) {
        wgx::hash_combine(seed, std::hash<void*>{}(layout.Get()));
    }
    return seed;
}

std::size_t BindGroupLayoutKeyHash::operator()(const BindGroupLayoutKey& key) const {
    std::size_t seed = 0;
    for (const auto& entry : key.entries) {
        wgx::hash_combine(seed, entry.binding);
        wgx::hash_combine(seed, static_cast<size_t>(entry.visibility));
        wgx::hash_combine(seed, entry.bindingArraySize);

        if (entry.buffer.type != wgpu::BufferBindingType::Undefined) {
            wgx::hash_combine(seed, static_cast<size_t>(entry.buffer.type));
            wgx::hash_combine(seed, entry.buffer.minBindingSize);
            wgx::hash_combine(seed, entry.buffer.hasDynamicOffset);
        } else if (entry.texture.sampleType != wgpu::TextureSampleType::Undefined) {
            wgx::hash_combine(seed, static_cast<size_t>(entry.texture.sampleType));
            wgx::hash_combine(seed, static_cast<size_t>(entry.texture.viewDimension));
            wgx::hash_combine(seed, static_cast<size_t>(entry.texture.multisampled));
        } else if (entry.sampler.type != wgpu::SamplerBindingType::Undefined) {
            wgx::hash_combine(seed, static_cast<size_t>(entry.sampler.type));
        } else if (entry.storageTexture.access != wgpu::StorageTextureAccess::Undefined) {
            wgx::hash_combine(seed, static_cast<size_t>(entry.storageTexture.access));
            wgx::hash_combine(seed, static_cast<size_t>(entry.storageTexture.format));
            wgx::hash_combine(seed, static_cast<size_t>(entry.storageTexture.viewDimension));
        }
    }
    return seed;
}
}  // namespace core::render