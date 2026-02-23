#pragma once
#include <Common.h>
#include <vector>
#include "Mesh.h"

namespace core::render {

struct RenderUnit {
    Handle meshHandle;
    uint32_t subMeshIndex = 0;

    Handle materialHandle;
    glm::mat4 modelMatrix = glm::mat4(1.0f);
};
struct Model {
    Model() = default;

    Model(Model&&) noexcept = default;
    Model& operator=(Model&&) noexcept = default;

    Model(const Model&) = delete;
    Model& operator=(const Model&) = delete;

    std::vector<RenderUnit> renderUnits;
};
}  // namespace core::render