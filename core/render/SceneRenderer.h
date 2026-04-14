#pragma once
#include <span>
#include "Scene.h"
#include "render.h"


namespace core::render {

struct RenderIntent {
    Handle meshHandle;
    uint32_t subMeshIndex;
    Handle materialHandle;
    uint32_t transformIndex;
    // TODO(#10): Populate 64-bit sort key for Radix Sorting
    uint64_t sortKey = 0;
};

struct RenderQueue {
    std::vector<RenderIntent> renderIntents;
    std::span<const glm::mat4x4> transforms;
    CameraUniformData cameraData;

    void Clear() {
        renderIntents.clear();
        transforms = {};
    }
};

class SceneRenderer {
  public:
    static void ExtractRenderQueue(const Scene& scene, RenderQueue& outRenderQueue);
};
}  // namespace core::render