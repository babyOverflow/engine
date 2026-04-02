#pragma once
#include "LayoutCache.h"
#include "Mesh.h"
#include "VertexLayoutManager.h"
#include "ResourcePool.h"
#include "ShaderAsset.h"
#include "render.h"

namespace core::render {

struct PipelineDesc {
    wgpu::StringView label = {};
    AssetView<ShaderAsset> shaderAsset;
    std::string vertexEntry;
    std::string fragmentEntry;
    wgpu::PrimitiveState primitive = wgx::PrimitiveState::kDefault;
    wgpu::BlendState blendState = wgx::BlendState::kReplace;
    MeshAssetFormat::MeshVertexState vertexState;

    bool operator==(const PipelineDesc& other) const {
        if (shaderAsset != other.shaderAsset) {
            return false;
        }
        if (vertexEntry != other.vertexEntry)
        {
            return false;
        }
        if (fragmentEntry != other.fragmentEntry)
        {
            return false;
        }
        if (!wgx::Equals(primitive, other.primitive)) {
            return false;
        }
        if (!wgx::Equals(blendState, other.blendState)) {
            return false;
        }
        if (vertexState != other.vertexState) {
            return false;
        }
        return true;
    }
};

struct PipelineDescHash {
    std::size_t operator()(const PipelineDesc& k) const {
        size_t seed = 0;
        wgx::hash_combine(seed, std::hash<Handle>{}(k.shaderAsset.handle));
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

     wgpu::RenderPipeline GetRenderPipeline(const PipelineDesc& Desc);

  private:
    Device* m_device;
    VertexLayoutManager m_vertexLayoutManager;

    wgpu::BindGroupLayout m_globalBindGroupLayout;

    std::unordered_map<PipelineDesc, wgpu::RenderPipeline, PipelineDescHash> m_pipelineCache;
    LayoutCache* m_layoutCache;
};
}  // namespace core::render