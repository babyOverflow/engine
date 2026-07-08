#include "DeferredGBufferPass.h"

void core::render::pass::DeferredGBufferPass::Execute(wgpu::RenderPassEncoder encoder,
                                                      const PassExecuteContext& executeContext) {
    wgpu::RenderPipeline pipeline = nullptr;
    wgpu::BindGroup materialBindGroup;
    for (uint32_t i = 0; i < executeContext.intents.size(); ++i) {
        const RenderIntent& intent = executeContext.intents[i];

        if (pipeline.Get() != intent.pipeline.Get()) {
            pipeline = intent.pipeline;
            encoder.SetPipeline(pipeline);
        }
        for (uint i = 0; i < intent.bufferRange.size(); ++i) {
            const auto bufferRange = intent.bufferRange[i];
            encoder.SetVertexBuffer(i, intent.vertexBuffer, bufferRange.offset, bufferRange.size);
        }
        encoder.SetIndexBuffer(intent.indexBuffer, wgpu::IndexFormat::Uint32);

        if (materialBindGroup.Get() != intent.bindGroup.Get()) {
            materialBindGroup = intent.bindGroup;
            encoder.SetBindGroup(BindSlot::Material, materialBindGroup);
        }

        encoder.DrawIndexed(intent.subMeshInfo.indexCount, 1, intent.subMeshInfo.indexStart);
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
