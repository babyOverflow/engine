#pragma once
#include "LayoutCache.h"
#include "Mesh.h"
#include "ResourcePool.h"
#include "render.h"

namespace core::render {

struct PipelineDesc {
    wgpu::StringView label = {};
    ShaderAsset* shaderAsset;
    VertexType vertexType = VertexType::None;
    wgpu::PrimitiveState primitive = wgx::PrimitiveState::kDefault;
    wgpu::BlendState blendState = wgx::BlendState::kReplace;

    bool operator==(const PipelineDesc& other) const {

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
        wgx::hash_combine(seed, std::hash<void*>{}(static_cast<void*>(k.shaderAsset)));
        wgx::hash_combine(seed, std::hash<int>{}(static_cast<int>(k.vertexType)));
        wgx::hash_combine(seed, wgx::Hash(k.primitive));
        wgx::hash_combine(seed, wgx::Hash(k.blendState));
        return seed;
    }
};

class PipelineManager {
  public:
    PipelineManager(Device* device,
                    LayoutCache* layoutCache,

                    wgpu::BindGroupLayoutDescriptor& globalBindGroupLayoutDesc);

    const GpuRenderPipeline* GetRenderPipeline(const PipelineDesc& Desc);
    wgpu::BindGroupLayout GetBindGroupLayout(wgpu::BindGroupLayoutDescriptor& bindGroupLayoutDesc);

  private:
    Device* m_device;

    wgpu::BindGroupLayout m_globalBindGroupLayout;

    std::unordered_map<PipelineDesc, GpuRenderPipeline, PipelineDescHash> m_pipelineCache;
    LayoutCache *m_layoutCache;
};
}  // namespace core::render