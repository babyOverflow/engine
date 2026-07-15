#include "ForwardRenderPass.h"

void core::render::pass::ForwardRenderPass::Setup(core::render::PassSetupContext& context) {
    Handle depthStencilHandle = context.DeclareTexture(
        "depthStencil", TextureDescriptor{
                            .usage = wgpu::TextureUsage::RenderAttachment,
                            .dimension = wgpu::TextureDimension::e2D,
                            .size = RelativeSize{1.0f, 1.0f},
                            .format = wgpu::TextureFormat::Depth24PlusStencil8,
                        });

    context.RegisterPassOutputs({{PassSetupContext::kSceneColorHandle}},
                                DepthStencilAttachment{depthStencilHandle});
}

void core::render::pass::ForwardRenderPass::Execute(wgpu::RenderPassEncoder encoder,
                                                    const PassExecuteContext& executeContext) {
    wgpu::RenderPipeline pipeline = nullptr;
    wgpu::BindGroup materialBindGroup = nullptr;
    for (uint32_t i = 0; i < executeContext.intents.size(); ++i) {
        const auto& intent = executeContext.intents[i];

        if (pipeline.Get() != intent.pipeline.Get()) {
            pipeline = intent.pipeline;
            encoder.SetPipeline(pipeline);
        }
        for (uint32_t j = 0; j < intent.bufferRange.size(); ++j) {
            const auto bufferRange = intent.bufferRange[j];
            encoder.SetVertexBuffer(j, intent.vertexBuffer, bufferRange.offset, bufferRange.size);
        }
        encoder.SetIndexBuffer(intent.indexBuffer, wgpu::IndexFormat::Uint32);
        if (materialBindGroup.Get() != intent.bindGroup.Get()) {
            materialBindGroup = intent.bindGroup;
            encoder.SetBindGroup(BindSlot::Material, materialBindGroup);
        }

        encoder.DrawIndexed(intent.subMeshInfo.indexCount, 1, intent.subMeshInfo.indexStart);
    }
}
