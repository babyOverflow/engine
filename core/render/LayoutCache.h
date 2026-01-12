#pragma once
#include <dawn/webgpu_cpp.h>

#include "render.h"

namespace core::render {

bool IsEntryEqual(const wgpu::BindGroupLayoutEntry& a, const wgpu::BindGroupLayoutEntry& b);

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

    wgpu::PipelineLayout GetPipelineLayout(std::vector<wgpu::BindGroupLayout> bindGroupLayouts);

  private:
    Device* m_device;

    std::unordered_map<BindGroupLayoutKey, GpuBindGroupLayout, BindGroupLayoutKeyHash>
        m_bindGroupLayoutCache;
    std::unordered_map<PipelineLayoutKey, GpuPipelineLayout, PipelineLayoutKeyHash>
        m_pipelineLayoutCache;
};

}  // namespace core::render