#pragma once
#include <span>
#include "AssetManager.h"
#include "IRenderPass.h"
#include "Scene.h"
#include "render.h"

namespace core::render {


struct RenderQueue {
    std::vector<RenderIntent> renderIntents;
    std::span<const glm::mat4x4> transforms;
    CameraUniformData cameraData;

    void Clear() {
        renderIntents.clear();
        transforms = {};
    }
};

class SceneCuller {
  public:
    static void ExtractRenderQueue(const Scene& scene,
                                   AssetManager* assetManager,
                                   PipelineManager* pipelineManager,
                                   RenderQueue& outRenderQueue);
};
}  // namespace core::render