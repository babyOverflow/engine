#include "ForwardRenderPass.h"

void core::render::pass::ForwardRenderPass::Setup(core::render::PassSetupContext& context) {
    Handle depthStencilHandle = context.DeclareTexture(
        "depthStencil", TextureDescriptor{
                            .usage = wgpu::TextureUsage::RenderAttachment,
                            .dimension = wgpu::TextureDimension::e2D,
                            .size = RelativeSize{1.0f, 1.0f},
                            .format = wgpu::TextureFormat::Depth24PlusStencil8,
                        });

    context.RegisterPassOutputs({{PassSetupContext::kSceneColorHandle}}, DepthStencilAttachment{depthStencilHandle});
}

void core::render::pass::ForwardRenderPass::Execute(wgpu::RenderPassEncoder pass,
                                                    const PassExecuteContext& executeContext) {
    for (uint32_t i = 0; i < executeContext.intents.size(); ++i) {
        const auto& intent = executeContext.intents[i];
        const Material& material =
            executeContext.assetRegistry.materials[intent.materialHandle.index];

        auto pipeline = intent.pipeline;
        pass.SetPipeline(pipeline);
        for (uint i = 0; i < intent.bufferRange.size(); ++i) {
            const auto bufferRange = intent.bufferRange[i];
            pass.SetVertexBuffer(i, intent.vertexBuffer, bufferRange.offset, bufferRange.size);
        }
        pass.SetIndexBuffer(intent.indexBuffer, wgpu::IndexFormat::Uint32);
        pass.SetBindGroup(BindSlot::Material, intent.bindGroup);

        pass.DrawIndexed(intent.subMeshInfo.indexCount, 1, intent.subMeshInfo.indexStart);
    }
}
