#include "ForwardRenderPass.h"

void core::render::pass::ForwardRenderPass::Setup(PassSetupContext& context)  {
    context.RegisterColorOutput(PassSetupContext::kSceneColorHandle,
                                PassSetupContext::ColorAttachment{
                                    .loadOp = wgpu::LoadOp::Clear,
                                    .storeOp = wgpu::StoreOp::Store,
                                    .clearValue = {1.0f, 1.0f, 1.0f, 1.0f},
                                });
    Handle depthStencilHandle = context.DeclareTexture(PassSetupContext::TextureDescriptor{
        .name = "depthStencil",
        .usage = wgpu::TextureUsage::RenderAttachment,
        .dimension = wgpu::TextureDimension::e2D,
        .size = PassSetupContext::RelativeSize{1.0f, 1.0f},
        .format = wgpu::TextureFormat::Depth24PlusStencil8,
    });

    context.RegisterDepthStencil(depthStencilHandle, PassSetupContext::DepthStencilAttachment{
                                                         .depthLoadOp = wgpu::LoadOp::Clear,
                                                         .depthStoreOp = wgpu::StoreOp::Store,
                                                         .depthClearValue = 1.0f});
}

void core::render::pass::ForwardRenderPass::Execute(
    wgpu::RenderPassEncoder pass,
    std::span<RenderIntent> context,
    const AssetRegistry& assetRegistry,
    std::span<const wgpu::RenderPipeline> pipelines) {
    for (uint32_t i = 0; i < context.size(); ++i) {
        const auto& intent = context[i];
        const Mesh& mesh = assetRegistry.meshes[intent.meshHandle.index];
        const Material& material = assetRegistry.materials[intent.materialHandle.index];

        const MeshAssetFormat::SubMeshInfo& subMesh = mesh.GetSubMeshInfo(intent.subMeshIndex);
        auto pipeline = pipelines[intent.sortKey.bits.pipelineId];
        pass.SetPipeline(pipeline);
        std::span<const MeshAssetFormat::BufferRange> bufferRanges =
            mesh.GetBufferRanges(subMesh.bufferRangeStart, subMesh.bufferRangeCount);
        for (uint i = 0; i < bufferRanges.size(); ++i) {
            const auto bufferRange = bufferRanges[i];
            pass.SetVertexBuffer(i, mesh.vertexBuffer, bufferRange.offset, bufferRange.size);
        }
        pass.SetIndexBuffer(mesh.indexBuffer, wgpu::IndexFormat::Uint32);
        wgpu::BindGroup bindGroup = material.GetBindGroup();
        pass.SetBindGroup(BindSlot::Material, bindGroup);

        pass.DrawIndexed(subMesh.indexCount, 1, subMesh.indexStart);
    }
}
