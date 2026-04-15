#pragma once
#include "render/render.h"
#include "render/Model.h"

namespace core {
struct Scene {
    render::CameraUniformData cameraData;
    std::vector<AssetView<render::Model>> models;
    std::vector<glm::mat4x4> modelMatrices;

    void AddModel(const AssetView<render::Model>& model, const glm::mat4x4& transform) {
        models.push_back(model);
        modelMatrices.push_back(transform);
    }
};
}  // namespace core