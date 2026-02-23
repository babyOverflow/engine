#pragma once

#include <glm/glm.hpp>

#include "Common.h"

namespace core {

    
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
    glm::vec4 tangent;
};

enum class VertexType {
    None = 0,
    StandardMesh = 1,
};

struct MeshAssetFormat {
    struct SubMeshInfo {
        uint32_t indexCount = 0;
        uint32_t baseIndexLocation = 0;
        uint32_t baseVertexLocation = 0;
        VertexType vertexType = VertexType::None;
    };
    std::vector<SubMeshInfo> subMeshes;
    std::vector<uint32_t> indexData;
    std::vector<std::byte> vertexData;
};
}  // namespace core