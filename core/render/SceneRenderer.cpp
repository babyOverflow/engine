
#include "SceneRenderer.h"
#include "PipelineManager.h"
void core::render::SceneCuller::ExtractRenderQueue(const Scene& scene,
                                                     AssetManager* assetManager,
                                                     PipelineManager* pipelineManager,
                                                     RenderQueue& outRenderQueue) {
    const auto& assetRegistry = assetManager->GetRegistry();
    for (uint32_t i = 0; i < scene.models.size(); ++i) {
        RenderIntent intent;
        for (const auto& renderUnit : scene.models[i]->renderUnits) {
            PipelineKey key;
            key.bits.shaderId =
                assetManager->GetMaterial(renderUnit.materialHandle)->GetShader().handle.index;
            key.bits.layoutId = assetManager->GetMesh(renderUnit.meshHandle)
                                    ->GetGlobalVertexStateID(renderUnit.subMeshIndex);
            key.bits.blendId =
                static_cast<uint64_t>(BlendMode::Opaque);  // TODO: support blend mode in material
            key.bits.depthStencilId =
                DepthStencilStateManager::kDefaultDepthStateID;  // TODO: support depth state in
                                                                 // material
            key.bits.passId = 0;  // TODO: support pass id in material
            key.bits.blendId = 0;
            key.bits.topology = static_cast<uint64_t>(wgpu::PrimitiveTopology::TriangleList);
            key.bits.cullMode = static_cast<uint64_t>(wgpu::CullMode::Back);
            key.bits.frontFace = static_cast<uint64_t>(wgpu::FrontFace::CCW);

            intent.meshHandle = renderUnit.meshHandle;
            intent.subMeshIndex = renderUnit.subMeshIndex;
            intent.materialHandle = renderUnit.materialHandle;
            intent.transformIndex = i;
            intent.pipelineKey = key;
            intent.sortKey.bits.pipelineId =
                pipelineManager->GetPipelineID(key, assetRegistry).index;
            // SceneCuller.cpp (Inside ExtractRenderQueue)
            // TODO(#10): Generate sort key from Pass, Pipeline, and Material IDs.
            // intent.sortKey.bits.passId = key.bits.passId;
            // intent.sortKey.bits.pipelineId =
            //    pipelineManager->GetPipelineID(key, assetManager->GetRegistry()).index;
            outRenderQueue.renderIntents.push_back(intent);
        }
    }

    outRenderQueue.cameraData = scene.cameraData;
    outRenderQueue.transforms = std::span(scene.modelMatrices.data(), scene.modelMatrices.size());
}