#pragma once
#include "LayoutCache.h"
#include "ResourcePool.h"
#include "render/resource/ShaderAsset.h"
#include "render/resource/MaterialManager.h"
#include "render/resource/VertexLayoutManager.h"
#include "render/render.h"

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
    const wgpu::DepthStencilState* GetDepthStencilState(uint64_t id) const {
        assert(id < m_depthStencilStates.size());
        if (id == kNullStateID) {
            return nullptr;
        }
        return &m_depthStencilStates[id];
    }

    constexpr static uint64_t kNullStateID = 0;
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
                    MaterialManager* materialManager,
                    PassManager* passManager,
                    VertexLayoutManager* vertexLayoutManager,
                    wgpu::BindGroupLayoutDescriptor& globalBindGroupLayoutDesc);

    struct PipelineConfig {
        AssetView<ShaderAsset> shader;
        uint32_t layoutId = 0;
        BlendMode blendMode = BlendMode::Opaque;
        uint32_t depthStencilId = 0;
        uint32_t passId = 0;
        wgpu::CullMode cullMode = wgpu::CullMode::Undefined;
        const PassTargetState* targetState;
    };

    Handle GetOrCreatePipeline(const PipelineConfig& config);
    // wgpu::RenderPipeline GetRenderPipeline(const PipelineDesc& Desc);
    std::span<const wgpu::RenderPipeline> GetAllPipelines() {
        return m_pipelinePool.GetDataSpan();
    };

    wgpu::RenderPipeline GetPipeline(Handle handle) { return GetAllPipelines()[handle.index]; }

  private:
    Device* m_device;
    LayoutCache* m_layoutCache;
    VertexLayoutManager* m_vertexLayoutManager;
    MaterialManager* m_materialManager;
    PassManager* m_passManager;
    DepthStencilStateManager m_depthStencilStateManager;

    wgpu::BindGroupLayout m_globalBindGroupLayout;

    std::unordered_map<uint64_t, Handle> m_pipelineIDCache;
    ResourcePool<wgpu::RenderPipeline> m_pipelinePool;
    // std::unordered_map<PipelineDesc, wgpu::RenderPipeline, PipelineDescHash> m_pipelineCache;
};
}  // namespace core::render
