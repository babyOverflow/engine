#pragma once
#include "ResourcePool.h"
#include "render.h"
#include "LayoutCache.h"

namespace core::render {

enum class VertexType {
    None = 0,
    StandardMesh = 1,
};



struct PipelineDesc {
    wgpu::StringView label = {};
    GpuShaderModule* shader;
    VertexType vertexType = VertexType::None;
    wgpu::PrimitiveState primitive = wgx::PrimitiveState::kDefault;
    wgpu::BlendState blendState = wgx::BlendState::kReplace;

    bool operator==(const PipelineDesc& other) const {
        if (shader != other.shader) {
            return false;
        }
        if (vertexType != other.vertexType) {
            return false;
        }

        if (!wgx::Equals(primitive, other.primitive)) {
            return false;
        }
        if (!wgx::Equals(blendState, other.blendState)) {
            return false;
        }
        return true;
    }
};

struct PipelineDescHash {
    std::size_t operator()(const PipelineDesc& k) const {
        size_t seed = 0;
        wgx::hash_combine(seed, std::hash<void*>{}(static_cast<void*>(k.shader)));
        wgx::hash_combine(seed, std::hash<int>{}(static_cast<int>(k.vertexType)));
        wgx::hash_combine(seed, wgx::Hash(k.primitive));
        wgx::hash_combine(seed, wgx::Hash(k.blendState));
        return seed;
    }
};

class PipelineManager {
  public:
    PipelineManager(Device* device, GpuBindGroupLayout* globalBindGroupLayout)
        : m_device(device), m_globalBindGroupLayout(globalBindGroupLayout), m_layoutCache(LayoutCache(device)) {}

    const GpuRenderPipeline* GetRenderPipeline(const PipelineDesc& Desc);

  private:
    Device* m_device;
    GpuBindGroupLayout* m_globalBindGroupLayout;

    
    std::unordered_map<PipelineDesc, GpuRenderPipeline, PipelineDescHash> m_pipelineCache;
    LayoutCache m_layoutCache;
};
}  // namespace core::render