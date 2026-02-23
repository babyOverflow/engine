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
        wgpu::RenderPipeline pipeline = m_pipelineManager->GetRenderPipeline(PipelineDesc{
            .shaderAsset = p.material->GetShader(),
            .vertexType = VertexType::StandardMesh,
        });
        pass.SetPipeline(pipeline);
        pass.SetVertexBuffer(0, p.vertexBuffer);
        pass.SetIndexBuffer(p.indexBuffer, wgpu::IndexFormat::Uint32);
        pass.SetBindGroup(1, p.material->GetBindGroup());
        //pass.SetBindGroup(2, m_tempBindGroup.bindGroup);

        pass.DrawIndexed(p.indexCount);
    }

    pass.End();
    auto commandBuffer = commandEncoder.Finish();
    d.GetQueue().Submit(1, &commandBuffer);
}
