#include "SceneRenderer.h"

void core::render::SceneRenderer::ExtractRenderQueue(const Scene& scene,
                                                     RenderQueue& outRenderQueue) {
    for (uint32_t i = 0; i < scene.models.size(); ++i) {
        RenderIntent intent;
        for (const auto& renderUnit : scene.models[i]->renderUnits) {
            intent.meshHandle = renderUnit.meshHandle;
            intent.subMeshIndex = renderUnit.subMeshIndex;
            intent.materialHandle = renderUnit.materialHandle;
            intent.transformIndex = i;
            // SceneRenderer.cpp (Inside ExtractRenderQueue)
            // TODO(#10): Generate sort key from Pass, Pipeline, and Material IDs.
            intent.sortKey = 0;
            outRenderQueue.renderIntents.push_back(intent);
        }
    }
    outRenderQueue.cameraData = scene.cameraData;
    outRenderQueue.transforms = std::span(scene.modelMatrices.data(), scene.modelMatrices.size());
}