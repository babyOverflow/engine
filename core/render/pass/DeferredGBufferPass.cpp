#include "DeferredGBufferPass.h"

void core::render::pass::DeferredGBufferPass::Execute(wgpu::RenderPassEncoder encoder,
                                                      const PassExecuteContext& executeContext) {
    for (uint32_t i = 0; i < executeContext.intents.size(); ++i) {
        const auto& intent = executeContext.intents[i];
        const Mesh& mesh = executeContext.assetRegistry.meshes[intent.meshHandle.index];

        const MeshAssetFormat::SubMeshInfo& subMesh = mesh.GetSubMeshInfo(intent.subMeshIndex);
        auto pipeline = intent.pipeline;
        encoder.SetPipeline(pipeline);
        std::span<const MeshAssetFormat::BufferRange> bufferRanges =
            mesh.GetBufferRanges(subMesh.bufferRangeStart, subMesh.bufferRangeCount);
        for (uint i = 0; i < bufferRanges.size(); ++i) {
            const auto bufferRange = bufferRanges[i];
            encoder.SetVertexBuffer(i, mesh.vertexBuffer, bufferRange.offset, bufferRange.size);
        }
        encoder.SetIndexBuffer(mesh.indexBuffer, wgpu::IndexFormat::Uint32);
        wgpu::BindGroup bindGroup = intent.bindGroup;
        encoder.SetBindGroup(BindSlot::Material, bindGroup);

        encoder.DrawIndexed(subMesh.indexCount, 1, subMesh.indexStart);
    }
}

void core::render::pass::DeferredGBufferPass::Setup(core::render::PassSetupContext& context) {

    Handle albedoHandle = context.DeclareTexture(
        "deferredGBufferAlbedo",
        TextureDescriptor{
            .label = "GBufferAlbedo",
            .usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding,
            .dimension = wgpu::TextureDimension::e2D,
            .size = RelativeSize{1.0f, 1.0f},
            .format = wgpu::TextureFormat::RGBA8Unorm,
        });
    Handle materialHandle = context.DeclareTexture(
        "deferredGBufferMaterial",
        TextureDescriptor{
            .label = "GBufferMaterial",
            .usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding,
            .dimension = wgpu::TextureDimension::e2D,
            .size = RelativeSize{1.0f, 1.0f},
            .format = wgpu::TextureFormat::RGBA16Float,
        });
    Handle normalHandle = context.DeclareTexture(
        "deferredGBufferNormal",
        TextureDescriptor{
            .label = "GBufferNormal",
            .usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding,
            .dimension = wgpu::TextureDimension::e2D,
            .size = RelativeSize{1.0f, 1.0f},
            .format = wgpu::TextureFormat::RGBA16Float,
        });

    Handle depthStencilHandle = context.DeclareTexture(
        "depthStencil",
        TextureDescriptor{
            .usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding,
            .dimension = wgpu::TextureDimension::e2D,
            .size = RelativeSize{1.0f, 1.0f},
            .format = wgpu::TextureFormat::Depth24PlusStencil8,
        });

    context.RegisterPassOutputs({{albedoHandle}, {materialHandle}, {normalHandle}},
                                DepthStencilAttachment{depthStencilHandle});
}
