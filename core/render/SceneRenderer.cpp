
#include "SceneRenderer.h"
#include "PipelineManager.h"
void core::render::SceneCuller::ExtractRenderQueue(const Scene& scene,
                                                     AssetManager* assetManager,
                                                     PipelineManager* pipelineManager,
                                                     RenderQueue& outRenderQueue) {
    const auto& assetRegistry = assetManager->GetRegistry();
    for (uint32_t i = 0; i < scene.models.size(); ++i) {
        for (const auto& renderUnit : scene.models[i]->renderUnits) {
            auto material = assetManager->GetMaterial(renderUnit.materialHandle);
            
            for (uint8_t passId : material->GetActivePassIDs()) {
                PipelineKey key;
                key.bits.shaderId = material->GetShader().handle.index;
                key.bits.layoutId = assetManager->GetMesh(renderUnit.meshHandle)
                                        ->GetGlobalVertexStateID(renderUnit.subMeshIndex);
                key.bits.blendId =
                    static_cast<uint64_t>(BlendMode::Opaque);  // TODO: support blend mode in material
                key.bits.depthStencilId =
                    DepthStencilStateManager::kDefaultDepthStateID;  // TODO: support depth state in
                                                                     // material
                key.bits.passId = passId;
                key.bits.topology = static_cast<uint64_t>(wgpu::PrimitiveTopology::TriangleList);
                key.bits.cullMode = static_cast<uint64_t>(wgpu::CullMode::Back);
                key.bits.frontFace = static_cast<uint64_t>(wgpu::FrontFace::CCW);

                RenderIntent intent;
                intent.meshHandle = renderUnit.meshHandle;
                intent.subMeshIndex = renderUnit.subMeshIndex;
                intent.materialHandle = renderUnit.materialHandle;
                intent.transformIndex = i;
                intent.pipelineKey = key;
                intent.sortKey.bits.pipelineId =
                    pipelineManager->GetPipelineID(key, assetRegistry).index;
                intent.sortKey.bits.passId = passId;

                outRenderQueue.renderIntents.push_back(intent);
            }
        }
    }

    outRenderQueue.cameraData = scene.cameraData;
    outRenderQueue.transforms = std::span(scene.modelMatrices.data(), scene.modelMatrices.size());
}