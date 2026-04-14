#pragma once

#include <webgpu/webgpu_cpp.h>
#include <string>

#include "AssetManager.h"
#include "render.h"
#include "PipelineManager.h"
#include "SceneRenderer.h"
#include "Material.h"

namespace core::render {

struct RenderPacket {
    wgpu::RenderPipeline pipeline;
    wgpu::Buffer vertexBuffer;
    wgpu::Buffer indexBuffer;
    std::span<const MeshAssetFormat::BufferRange> bufferRanges;
    uint32_t indexStart = 0; 
    uint32_t indexCount = 0;
    AssetView<Material> material;
};

class FrameContext {
  public:
    void Submit(const RenderPacket& packet) { m_drawQueue.push_back(packet); }
    const std::vector<RenderPacket>& GetQueue() const { return m_drawQueue; }
    void ClearQueue() { m_drawQueue.clear(); }

    void SetCameraData(CameraUniformData& cameraData) { m_cameraData = cameraData;}
    CameraUniformData& GetCameraUniformData() { return m_cameraData; }
  private:
    std::vector<RenderPacket> m_drawQueue;
    CameraUniformData m_cameraData;
};

class RenderGraph {
  public:
    RenderGraph(Device* device,
                AssetManager* assetManager,
                PipelineManager* pipelineManager,
                const wgpu::BindGroupLayout globalBindGroupLayout);

    RenderGraph(RenderGraph&& rhs) = default;

    void Execute(RenderQueue& frameContext);

  private:
    Device* m_device;

    AssetManager* m_assetManager;  // TODO!(#2, #8; Sunghyun) m_assetManager will be removed in
                                   // future, and each RenderPass will directly access asset manager
                                   // to get necessary resource. But for now, we put asset manager
                                   // in RenderGraph for simplicity.
    PipelineManager* m_pipelineManager;

    wgpu::BindGroup m_globalBindGroup;
    wgpu::Buffer m_globalUniformBuffer;

 
};
}  // namespace core::render