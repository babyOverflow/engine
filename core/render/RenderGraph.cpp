#include "RenderGraph.h"
#include <array>

core::render::RenderGraph::RenderGraph(Device* device,
                                       PipelineManager* pipelineManager,
                                       const wgpu::BindGroupLayout globalBindGroupLayout)
    : m_device(device), m_pipelineManager(pipelineManager) {
    CameraUniformData cameraUniformData;
    wgpu::Buffer globalUniform = device->CreateBufferFromData(&cameraUniformData.viewProj,
                                                              sizeof(cameraUniformData.viewProj),
                                                              wgpu::BufferUsage::Uniform);
    m_globalUniformBuffer = std::move(globalUniform);

    std::array<wgpu::BindGroupEntry, 1> bindGroupEntries{wgpu::BindGroupEntry{
        .binding = 0,
        .buffer = m_globalUniformBuffer,
        .offset = 0,
        .size = sizeof(cameraUniformData.viewProj),
    }};
    m_globalBindGroup = device->CreateBindGroup(wgpu::BindGroupDescriptor{
        .layout = globalBindGroupLayout,
        .entryCount = bindGroupEntries.size(),
        .entries = bindGroupEntries.data(),
    });

}

void core::render::RenderGraph::Execute(FrameContext& frameContext) {
    auto d = m_device->GetDeivce();

    wgpu::RenderPassColorAttachment colorAttachment{
        .view = m_device->GetCurrentTextureView(),
        .resolveTarget = nullptr,
        .loadOp = wgpu::LoadOp::Clear,
        .storeOp = wgpu::StoreOp::Store,
        .clearValue = {0.1f, 0.1f, 0.9f, 1.0f},
    };
    wgpu::RenderPassDescriptor renderPassDesc{
        .label = "Main Render Pass",
        .colorAttachmentCount = 1,
        .colorAttachments = &colorAttachment,
        .depthStencilAttachment = nullptr,
    };

    auto& cameraData = frameContext.GetCameraUniformData();
    d.GetQueue().WriteBuffer(m_globalUniformBuffer, 0, &cameraData.viewProj,
                             sizeof(cameraData.viewProj));

    auto commandEncoder = d.CreateCommandEncoder();
    auto pass = commandEncoder.BeginRenderPass(&renderPassDesc);

    pass.SetBindGroup(0, m_globalBindGroup);

    auto packets = frameContext.GetQueue();
    for (auto& p : packets) {
        pass.SetPipeline(p.pipeline);
        for (uint i = 0; i < p.bufferRanges.size(); ++i) {
            const auto bufferRange = p.bufferRanges[i];
            pass.SetVertexBuffer(i, p.vertexBuffer, bufferRange.offset, bufferRange.size);
        }

        pass.SetIndexBuffer(p.indexBuffer, wgpu::IndexFormat::Uint32);
        pass.SetBindGroup(1, p.material->GetBindGroup());
        // pass.SetBindGroup(2, m_tempBindGroup.bindGroup);

        pass.DrawIndexed(p.indexCount, 1, p.indexStart);
    }

    pass.End();
    auto commandBuffer = commandEncoder.Finish();
    d.GetQueue().Submit(1, &commandBuffer);
}
