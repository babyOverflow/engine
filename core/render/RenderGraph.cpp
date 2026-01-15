#include "RenderGraph.h"
#include <array>

core::render::RenderGraph::RenderGraph(Device* device, const wgpu::BindGroupLayout globalBindGroupLayout)
    : m_device(device) {


    CameraUniformData cameraUniformData;
    GpuBuffer globalUniform = device->CreateBufferFromData(&cameraUniformData.viewProj,
                                                           sizeof(cameraUniformData.viewProj),
                                                           wgpu::BufferUsage::Uniform);
    m_globalUniformBuffer = std::move(globalUniform);

    std::array<wgpu::BindGroupEntry, 1> bindGroupEntries{wgpu::BindGroupEntry{
        .binding = 0,
        .buffer = m_globalUniformBuffer.GetHandle(),
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
    d.GetQueue().WriteBuffer(m_globalUniformBuffer.GetHandle(), 0, &cameraData.viewProj,
                             sizeof(cameraData.viewProj));

    auto commandEncoder = d.CreateCommandEncoder();
    auto pass = commandEncoder.BeginRenderPass(&renderPassDesc);

    pass.SetBindGroup(0, m_globalBindGroup.GetHandle());

    auto packets = frameContext.GetQueue();
    for (auto& p : packets) {
        pass.SetPipeline(p.pipeline);
        pass.SetVertexBuffer(0, p.vertexBuffer);
        pass.SetIndexBuffer(p.indexBuffer, wgpu::IndexFormat::Uint32);

        pass.DrawIndexed(p.indexCount);
    }

    pass.End();
    auto commandBuffer = commandEncoder.Finish();
    d.GetQueue().Submit(1, &commandBuffer);
}
