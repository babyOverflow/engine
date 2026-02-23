#pragma once
#include "LayoutCache.h"
#include "Material.h"
#include "Mesh.h"
#include "ResourcePool.h"
#include "ShaderAsset.h"
#include "render.h"

namespace core::render {

struct PipelineDesc {
    wgpu::StringView label = {};
    AssetView<ShaderAsset> shaderAsset;
    VertexType vertexType = VertexType::None;
    wgpu::PrimitiveState primitive = wgx::PrimitiveState::kDefault;
    wgpu::BlendState blendState = wgx::BlendState::kReplace;

    bool operator==(const PipelineDesc& other) const {
        if (shaderAsset != other.shaderAsset) {
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
        wgx::hash_combine(seed, std::hash<Handle>{}(k.shaderAsset.handle));
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

     wgpu::RenderPipeline GetRenderPipeline(const PipelineDesc& Desc);

  private:
    Device* m_device;

    wgpu::BindGroupLayout m_globalBindGroupLayout;

    std::unordered_map<PipelineDesc, wgpu::RenderPipeline, PipelineDescHash> m_pipelineCache;
    LayoutCache* m_layoutCache;
};
}  // namespace core::render