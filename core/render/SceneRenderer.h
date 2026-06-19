#pragma once
#include <span>
#include "IRenderPass.h"
#include "Scene.h"
#include "ShaderManager.h"
#include "BindGroupManager.h"
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
                                   std::span<uint32_t> passes,
                                   AssetManager* assetManager,
                                   ShaderManager* shaderManager,
                                   PipelineManager* pipelineManager,
                                   BindGroupManager* bindGroupManager,
                                   RenderQueue& outRenderQueue);
};
}  // namespace core::render