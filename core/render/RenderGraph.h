#pragma once

#include <webgpu/webgpu_cpp.h>
#include <string>

#include "render.h"

namespace core::render {

struct RenderPacket {
    wgpu::RenderPipeline pipeline;
    wgpu::Buffer vertexBuffer;
    wgpu::Buffer indexBuffer;
    wgpu::BindGroup bindGroup;

    size_t indexCount;
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
    RenderGraph(Device* device, const wgpu::BindGroupLayout globalBindGroupLayout);

    RenderGraph(RenderGraph&& rhs) = default;

    void Execute(FrameContext& frameContext);

  private:
    Device* m_device;

    GpuBindGroup m_globalBindGroup;
    GpuBuffer m_globalUniformBuffer;
};
}  // namespace core::render