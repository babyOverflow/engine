#pragma once

#include "Common.h"

namespace core {
struct ModelAssetFormat {
    struct Node {
        core::AssetPath meshId;

        std::vector<core::AssetPath> materialIds;
        glm::mat4 localMatrix = glm::mat4(1.0f);
        std::string name;
    };

    std::vector<Node> nodes;
};
}  // namespace core