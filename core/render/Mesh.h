#pragma once

#include <array>

#include "render.h"

namespace core::render {

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
    glm::vec4 tangent;

    static const std::array<wgpu::VertexAttribute, 4> GetAttributes();
};

struct SubMesh {
    GpuBuffer vertexBuffer;
    GpuBuffer indexBuffer;
    size_t indexCount = 0;
    wgpu::IndexFormat indexFormat;
};

struct Model {
    std::vector<SubMesh> subMeshes;

    Model() = default;

    Model(Model&&) noexcept = default;
    Model& operator=(Model&&) noexcept = default;

    // 3. 복사 생성자 및 대입 연산자 금지 (권장 사항)
    // (SubMesh가 이미 복사 금지이므로, 명시적으로 적어주는 것이 안전합니다)
    Model(const Model&) = delete;
    Model& operator=(const Model&) = delete;
};

constexpr uint32_t kInvaildModelGen = -1;

}  // namespace core::render