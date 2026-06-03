#include "DeferredGBufferPass.h"

void core::render::pass::DeferredGBufferPass::Execute(
    wgpu::RenderPassEncoder encoder,
    std::span<RenderIntent> context,
    const AssetRegistry& assetRegistry,
    std::span<const wgpu::RenderPipeline> pipelines) {
    for (uint32_t i = 0; i < context.size(); ++i) {
        const auto& intent = context[i];
        const Mesh& mesh = assetRegistry.meshes[intent.meshHandle.index];
        const Material& material = assetRegistry.materials[intent.materialHandle.index];

        const MeshAssetFormat::SubMeshInfo& subMesh = mesh.GetSubMeshInfo(intent.subMeshIndex);
        auto pipeline = pipelines[intent.sortKey.bits.pipelineId];
        encoder.SetPipeline(pipeline);
        std::span<const MeshAssetFormat::BufferRange> bufferRanges =
            mesh.GetBufferRanges(subMesh.bufferRangeStart, subMesh.bufferRangeCount);
        for (uint i = 0; i < bufferRanges.size(); ++i) {
            const auto bufferRange = bufferRanges[i];
            encoder.SetVertexBuffer(i, mesh.vertexBuffer, bufferRange.offset, bufferRange.size);
        }
        encoder.SetIndexBuffer(mesh.indexBuffer, wgpu::IndexFormat::Uint32);
        wgpu::BindGroup bindGroup = material.GetBindGroup(intent.sortKey.bits.passId);
        encoder.SetBindGroup(BindSlot::Material, bindGroup);

        encoder.DrawIndexed(subMesh.indexCount, 1, subMesh.indexStart);
    }
}

void core::render::pass::DeferredGBufferPass::Setup(PassSetupContext& context) {
    Handle positonHandle = context.DeclareTexture(PassSetupContext::TextureDescriptor{
        .name = "deferredGBufferPosition",
        .usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding,
        .dimension = wgpu::TextureDimension::e2D,
        .size = PassSetupContext::RelativeSize{1.0f, 1.0f},
        .format = wgpu::TextureFormat::RGBA16Float,
    });

    Handle normalHandle = context.DeclareTexture(PassSetupContext::TextureDescriptor{
        .name = "deferredGBufferNormal",
        .usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding,
        .dimension = wgpu::TextureDimension::e2D,
        .size = PassSetupContext::RelativeSize{1.0f, 1.0f},
        .format = wgpu::TextureFormat::RGBA16Float,
    });
    Handle albedoHandle = context.DeclareTexture(PassSetupContext::TextureDescriptor{
        .name = "deferredGBufferAlbedo",
        .usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding,
        .dimension = wgpu::TextureDimension::e2D,
        .size = PassSetupContext::RelativeSize{1.0f, 1.0f},
        .format = wgpu::TextureFormat::RGBA8Unorm,
    });

    context.GetBlackBoard().Set("deferredGBufferPosition", positonHandle);
    context.GetBlackBoard().Set("deferredGBufferNormal", normalHandle);
    context.GetBlackBoard().Set("deferredGBufferAlbedo", albedoHandle);

    context.RegisterColorOutput(positonHandle, PassSetupContext::ColorAttachment{
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
}