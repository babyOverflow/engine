#pragma once
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include "render/render.h"
#include "render/resource/Model.h"

namespace core {

struct LocalTransformComponent {
    glm::vec3 position;
    glm::vec4 rotation;
    glm::vec3 scale;
};

struct WorldMatrixComponent {
    glm::mat4x4 transform;
};

struct BoundingBoxComponent {
    glm::vec3 min;
    glm::vec3 max;
};

struct StaticMeshComponent {
    Handle meshHandle;
    Handle materialHandle;
    uint32_t subMeshIndex;
};

struct SkinnedMeshComponent {
    Handle meshHandle;
    Handle materialHandle;
    Handle skeletonHandle;
};

struct MaterialInstanceComponent {
    uint32_t customDataIndex;
};

struct BoneOffsetComponent {
    uint32_t globalBufferOffset;
};

class Scene {
  public:
    render::CameraUniformData cameraData;
    entt::registry registry;
};
}  // namespace core