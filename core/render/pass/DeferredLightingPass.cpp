#include "DeferredLightingPass.h"

//void core::render::pass::DeferredLightingPass::Execute(
//    wgpu::RenderPassEncoder encoder,
//    std::span<RenderIntent> context,
//    const AssetRegistry& assetRegistry,
//    std::span<const wgpu::RenderPipeline> pipelines) {}
//
//void core::render::pass::DeferredLightingPass::Setup(PassSetupContext& context) {
//    Handle positionHandle = context.GetBlackBoard().Get("deferredGBufferPosition");
//    Handle normalHandle = context.GetBlackBoard().Get("deferredGBufferNormal");
//    Handle albedoHandle = context.GetBlackBoard().Get("deferredGBufferAlbedo");
//    context.RegisterTextureRead(positionHandle);
//    context.RegisterTextureRead(normalHandle);
//    context.RegisterTextureRead(albedoHandle);
//
//    context.RegisterColorOutput(PassSetupContext::kSceneColorHandle,
//                                {
//                                    .loadOp = wgpu::LoadOp::Clear,
//                                    .storeOp = wgpu::StoreOp::Store,
//                                    .clearValue = {1.0, 1.0, 1.0, 1.0},
//                                });
//}
