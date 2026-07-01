
#include "SceneRenderer.h"
#include "render/backend/BindGroupManager.h"
#include "render/backend/PipelineManager.h"
void core::render::SceneCuller::ExtractRenderQueue(const Scene& scene,
                                                   std::span<uint32_t> passes,
                                                   AssetManager* assetManager,
                                                   ShaderManager* shaderManager,
                                                   PipelineManager* pipelineManager,
                                                   BindGroupManager* bindGroupManager,
                                                   RenderQueue& outRenderQueue) {
    auto assetRegistry = assetManager->GetRegistry();
    for (uint32_t i = 0; i < scene.models.size(); ++i) {
        for (const auto& renderUnit : scene.models[i]->renderUnits) {
            AssetView<Material> material = assetManager->GetMaterial(renderUnit.materialHandle);
            for (uint32_t passId : passes) {
                PipelineKey key;
                Handle shaderHandle =
                    shaderManager->GetShaderHandle(passId, material->GetActiveTechniqueID());
                if (!shaderHandle.IsValid()) {
                    continue;
                }
                AssetView<ShaderAsset> shader = shaderManager->GetShaderAsset(shaderHandle);

                Handle pipelineHandle =
                    pipelineManager->GetOrCreatePipeline(PipelineManager::PipelineConfig{
                        .shader = shader,
                        .layoutId = assetManager->GetMesh(renderUnit.meshHandle)
                                        ->GetGlobalVertexStateID(renderUnit.subMeshIndex),
                        .blendMode = BlendMode::Opaque,
                        .depthStencilId = DepthStencilStateManager::kDefaultDepthStateID,
                        .passId = passId,
                        .cullMode = wgpu::CullMode::Back,
                    });

                RenderIntent intent;
                intent.meshHandle = renderUnit.meshHandle;
                intent.subMeshIndex = renderUnit.subMeshIndex;
                intent.materialHandle = renderUnit.materialHandle;
                intent.transformIndex = i;
                intent.pipeline = pipelineManager->GetPipeline(pipelineHandle);
                intent.sortKey = RenderIntent::CreateOpaqueKey(pipelineHandle.index,
                                                               renderUnit.materialHandle.index,
                                                               renderUnit.meshHandle.index, 0);

                intent.bindGroup = bindGroupManager->GetBindGroup(material.handle);

                outRenderQueue.renderIntents[passId].push_back(intent);
            }
        }
    }

    outRenderQueue.cameraData = scene.cameraData;
    outRenderQueue.transforms = std::span(scene.modelMatrices.data(), scene.modelMatrices.size());
}