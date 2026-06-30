#pragma once
#include <span>
#include "BindGroupManager.h"
#include "IRenderPass.h"
#include "Scene.h"
#include "ShaderManager.h"
#include "render.h"

namespace core::render {

struct RenderQueue {
    std::array<std::vector<RenderIntent>, PassManager::kMaxPasses> renderIntents;
    std::array<wgpu::RenderPipeline, PassManager::kMaxPasses> proceduralPipelines;
    std::span<const glm::mat4x4> transforms;
    CameraUniformData cameraData;

    void Clear() {
        for (uint32_t i = 0; i < PassManager::kMaxPasses; ++i) {
            renderIntents[i].clear();
        }
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