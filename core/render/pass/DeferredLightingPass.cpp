#include "DeferredLightingPass.h"

void core::render::pass::DeferredLightingPass::Execute(wgpu::RenderPassEncoder encoder,
                                                       const PassExecuteContext& executeContext) {
    encoder.SetPipeline(executeContext.proceduralPipeline);
    encoder.Draw(3, 1, 0, 0);
}

void core::render::pass::DeferredLightingPass::Setup(core::render::PassSetupContext& context) {
    Handle normalHandle = context.GetResourceHandle("deferredGBufferNormal");
    Handle albedoHandle = context.GetResourceHandle("deferredGBufferAlbedo");
    Handle mateiralHandle = context.GetResourceHandle("deferredGBufferMaterial");
    Handle depthHandle = context.GetResourceHandle("depthStencil");

    context.RegisterTextureRead(albedoHandle);
    context.RegisterTextureRead(mateiralHandle);
    context.RegisterTextureRead(normalHandle);
    context.RegisterTextureRead(
        depthHandle, wgpu::TextureViewDescriptor{.aspect = wgpu::TextureAspect::DepthOnly,
                                                 .usage = wgpu::TextureUsage::TextureBinding});

    context.RegisterPassOutputs({{PassSetupContext::kSceneColorHandle}});
}
