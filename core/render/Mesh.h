#pragma once

#include <array>

#include "render.h"

namespace core::render {

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
    glm::vec4 tangent;

    static wgpu::VertexBufferLayout GetWgpuVertexLayout();
};

enum class VertexType {
    None = 0,
    StandardMesh = 1,
};

wgpu::VertexBufferLayout MapVertexDesc(VertexType type);

struct SubMeshInfo {
    uint32_t indexCount;
    uint32_t indexOffset;
    uint32_t vertexOffset;
};

struct Mesh {
    wgpu::Buffer vertexBuffer;
    wgpu::Buffer indexBuffer;
    std::vector<SubMeshInfo> submeshInfos;
};

}  // namespace core::render