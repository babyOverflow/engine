#pragma once
#include "AssetManager.h"
#include "LayoutCache.h"
#include "Mesh.h"
#include "ResourcePool.h"
#include "ShaderAsset.h"
#include "VertexLayoutManager.h"
#include "render.h"

namespace core::render {
class PassManager;
enum class BlendMode : uint8_t {
    Opaque = 0,
    AlphaBlend = 1,
    Additive = 2,
    Premultiplied = 3,
    Multiply = 4
};

class DepthStencilStateManager {
  public:
    DepthStencilStateManager();
    uint64_t RegisterDepthStencilState(wgpu::DepthStencilState& state);
    wgpu::DepthStencilState GetDepthStencilState(uint64_t id) const {
        assert(id < m_depthStencilStates.size());
        return m_depthStencilStates[id];
    }

    constexpr static uint64_t kDefaultStateID = 0;
    constexpr static uint64_t kDefaultDepthStateID = 1;

  private:
    std::vector<wgpu::DepthStencilState> m_depthStencilStates;
};

union PipelineKey {
    uint64_t hash;  // Used for fast O(1) unordered_map lookup and == comparison

    struct {
        // --- Managed States (Handles/IDs) ---
        uint64_t shaderId : 16;       // 65,536 logical shaders/permutations
        uint64_t passId : 8;          // 256 render pass signatures (target formats)
        uint64_t layoutId : 8;        // 256 vertex layouts (Plenty, since N < 20)
        uint64_t blendId : 8;         // 256 blend states
        uint64_t depthStencilId : 8;  // 256 depth-stencil states

        // --- Inlined WebGPU States (No manager needed) ---
        uint64_t topology : 3;   // wgpu::PrimitiveTopology (PointList to TriangleStrip = 5 values)
        uint64_t cullMode : 2;   // wgpu::CullMode (None, Front, Back = 3 values)
        uint64_t frontFace : 1;  // wgpu::FrontFace (CCW, CW = 2 values)
        uint64_t sampleCount : 3;  // MSAA samples (1, 2, 4, 8)

        // --- Reserved for future (e.g., ColorWriteMask) ---
        uint64_t reserved : 7;  // Pads exactly to 64 bits
    } bits;
};

class PipelineManager {
  public:
    PipelineManager(Device* device,
                    LayoutCache* layoutCache,
        PassManager* passManager,
                    VertexLayoutManager* vertexLayoutManager,
                    wgpu::BindGroupLayoutDescriptor& globalBindGroupLayoutDesc);

    Handle GetPipelineID(PipelineKey key, AssetRegistry assetRegistry);
    // wgpu::RenderPipeline GetRenderPipeline(const PipelineDesc& Desc);
    std::span<const wgpu::RenderPipeline> GetAllPipelines() {
        return m_pipelinePool.GetDataSpan();
    };

  private:
    Device* m_device;
    VertexLayoutManager* m_vertexLayoutManager;
    PassManager* m_passManager;
    DepthStencilStateManager m_depthStencilStateManager;

    wgpu::BindGroupLayout m_globalBindGroupLayout;

    std::unordered_map<uint64_t, Handle> m_pipelineIDCache;
    ResourcePool<wgpu::RenderPipeline> m_pipelinePool;
    // std::unordered_map<PipelineDesc, wgpu::RenderPipeline, PipelineDescHash> m_pipelineCache;
    LayoutCache* m_layoutCache;
};
}  // namespace core::render