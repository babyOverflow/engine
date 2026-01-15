#pragma once
#include <dawn/webgpu_cpp.h>

#include "render.h"

namespace core::render {



struct BindGroupLayoutKey {
    std::vector<wgpu::BindGroupLayoutEntry> entries;
    static BindGroupLayoutKey From(wgpu::BindGroupLayoutDescriptor& desc);
    bool operator==(const BindGroupLayoutKey& a) const;
};

struct BindGroupLayoutKeyHash {
    std::size_t operator()(const BindGroupLayoutKey& key) const;
};

struct PipelineLayoutKey {
    std::vector<wgpu::BindGroupLayout> layouts;

    bool operator==(const PipelineLayoutKey& other) const;
};

struct PipelineLayoutKeyHash {
    std::size_t operator()(const PipelineLayoutKey& key) const;
};

class LayoutCache {
  public:
    LayoutCache(Device* device) : m_device(device) {}

    wgpu::BindGroupLayout GetBindGroupLayout(wgpu::BindGroupLayoutDescriptor& desc);

    wgpu::PipelineLayout GetPipelineLayout(wgpu::PipelineLayoutDescriptor& pipelineLayoutDesc);

  private:
    Device* m_device;

    std::unordered_map<BindGroupLayoutKey, GpuBindGroupLayout, BindGroupLayoutKeyHash>
        m_bindGroupLayoutCache;
    std::unordered_map<PipelineLayoutKey, GpuPipelineLayout, PipelineLayoutKeyHash>
        m_pipelineLayoutCache;
};

}  // namespace core::render