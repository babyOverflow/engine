#pragma once
#include <webgpu/webgpu_cpp.h>
namespace wgx {

inline bool IsCompatible(const wgpu::BindGroupLayoutEntry& a, const wgpu::BindGroupLayoutEntry& b) {
    if (a.binding != b.binding || a.bindingArraySize != b.bindingArraySize) {
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

inline bool IsEntryEqual(const wgpu::BindGroupLayoutEntry& a, const wgpu::BindGroupLayoutEntry& b) {
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

}