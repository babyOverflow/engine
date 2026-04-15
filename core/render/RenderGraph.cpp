#include "RenderGraph.h"
#include <array>
#include "Material.h"
#include "Mesh.h"
#include "SceneRenderer.h"

core::render::RenderGraph::RenderGraph(Device* device,
                                       AssetManager* assetManager,
                                       PipelineManager* pipelineManager,
                                       const wgpu::BindGroupLayout globalBindGroupLayout)
    : m_device(device), m_assetManager(assetManager), m_pipelineManager(pipelineManager) {
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

void core::render::RenderGraph::Execute(RenderQueue& renderQueue) {
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

    auto& cameraData = renderQueue.cameraData;
    d.GetQueue().WriteBuffer(m_globalUniformBuffer, 0, &cameraData.viewProj,
                             sizeof(cameraData.viewProj));

    auto commandEncoder = d.CreateCommandEncoder();
    auto pass = commandEncoder.BeginRenderPass(&renderPassDesc);

    pass.SetBindGroup(0, m_globalBindGroup);

    // TODO!(#2, #8; Sunghyun) beneath render logic will be replace by a RenderPass defined in
    // issues.
    for (uint32_t i = 0; i < renderQueue.renderIntents.size(); ++i) {
        const auto& intent = renderQueue.renderIntents[i];
        const auto& transform = renderQueue.transforms[intent.transformIndex];
        AssetView<Mesh> mesh = m_assetManager->GetMesh(intent.meshHandle);
        AssetView<Material> material = m_assetManager->GetMaterial(intent.materialHandle);

        const MeshAssetFormat::SubMeshInfo& subMesh = mesh->GetSubMeshInfo(intent.subMeshIndex);
        auto pipeline = m_pipelineManager->GetRenderPipeline(
            material->GetPipelineDesc(mesh->GetVertexState(subMesh.stateIndex)));
        pass.SetPipeline(pipeline);
        std::span<const MeshAssetFormat::BufferRange> bufferRanges =
            mesh->GetBufferRanges(subMesh.bufferRangeStart, subMesh.bufferRangeCount);
        for (uint i = 0; i < bufferRanges.size(); ++i) {
            const auto bufferRange = bufferRanges[i];
            pass.SetVertexBuffer(i, mesh->vertexBuffer, bufferRange.offset, bufferRange.size);
        }

        pass.SetIndexBuffer(mesh->indexBuffer, wgpu::IndexFormat::Uint32);
        pass.SetBindGroup(1, material->GetBindGroup());

        pass.DrawIndexed(subMesh.indexCount, 1, subMesh.indexStart);
    }

    pass.End();
    auto commandBuffer = commandEncoder.Finish();
    d.GetQueue().Submit(1, &commandBuffer);
}
