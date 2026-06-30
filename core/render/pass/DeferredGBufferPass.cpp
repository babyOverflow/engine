#include "DeferredGBufferPass.h"

void core::render::pass::DeferredGBufferPass::Execute(wgpu::RenderPassEncoder encoder, const PassExecuteContext& executeContext) {
    for (uint32_t i = 0; i < executeContext.intents.size(); ++i) {
        const auto& intent = executeContext.intents[i];
        const Mesh& mesh = executeContext.assetRegistry.meshes[intent.meshHandle.index];
        const Material& material =
            executeContext.assetRegistry.materials[intent.materialHandle.index];

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

const core::render::PassTargetState core::render::pass::DeferredGBufferPass::kSignature{
    .flags = PassFlags::ClearTargets,
    .colorTargetFormats =
        {
            wgpu::TextureFormat::RGBA8Unorm,
            wgpu::TextureFormat::RGBA16Float,
            wgpu::TextureFormat::RGBA16Float,
        },
    .depthStencilFormat = wgpu::TextureFormat::Depth24PlusStencil8,
};

void core::render::pass::DeferredGBufferPass::Setup(PassSetupContext& context) { 
    const PassTargetState& targetState = GetTargetState();

    Handle albedoHandle = context.DeclareTexture(
        "deferredGBufferAlbedo",
        PassSetupContext::TextureDescriptor{
            .label = "GBufferAlbedo",
            .usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding,
            .dimension = wgpu::TextureDimension::e2D,
            .size = PassSetupContext::RelativeSize{1.0f, 1.0f},
            .format = targetState.colorTargetFormats[0],
        });
    Handle materialHandle = context.DeclareTexture(
        "deferredGBufferMaterial",
        PassSetupContext::TextureDescriptor{
            .label = "GBufferMaterial",
            .usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding,
            .dimension = wgpu::TextureDimension::e2D,
            .size = PassSetupContext::RelativeSize{1.0f, 1.0f},
            .format = targetState.colorTargetFormats[1],
        });
    Handle normalHandle = context.DeclareTexture(
        "deferredGBufferNormal",
        PassSetupContext::TextureDescriptor{
            .label = "GBufferNormal",
            .usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding,
            .dimension = wgpu::TextureDimension::e2D,
            .size = PassSetupContext::RelativeSize{1.0f, 1.0f},
            .format = targetState.colorTargetFormats[2],
        });


    Handle depthStencilHandle = context.DeclareTexture(
        "depthStencil",
        PassSetupContext::TextureDescriptor{
            .usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding,
            .dimension = wgpu::TextureDimension::e2D,
            .size = PassSetupContext::RelativeSize{1.0f, 1.0f},
            .format = targetState.depthStencilFormat,
        });

    context.RegisterColorOutput(materialHandle, PassSetupContext::ColorAttachment{
                                                    .loadOp = wgpu::LoadOp::Clear,
                                                    .storeOp = wgpu::StoreOp::Store,
                                                    .clearValue = {0.0f, 0.0f, 0.0f, 1.0f},
                                                });
    context.RegisterColorOutput(normalHandle, PassSetupContext::ColorAttachment{
                                                  .loadOp = wgpu::LoadOp::Clear,
                                                  .storeOp = wgpu::StoreOp::Store,
                                                  .clearValue = {0.0f, 0.0f, 0.0f, 1.0f},
                                              });

    context.RegisterColorOutput(albedoHandle, PassSetupContext::ColorAttachment{
                                                  .loadOp = wgpu::LoadOp::Clear,
                                                  .storeOp = wgpu::StoreOp::Store,
                                                  .clearValue = {0.0f, 0.0f, 0.0f, 1.0f},
                                              });
    context.RegisterDepthStencil(depthStencilHandle, PassSetupContext::DepthStencilAttachment{
                                                         .depthLoadOp = wgpu::LoadOp::Clear,
                                                         .depthStoreOp = wgpu::StoreOp::Store,
                                                         .depthClearValue = 1.0f});
}