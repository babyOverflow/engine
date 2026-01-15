#include "Mesh.h"

namespace core::render {

wgpu::VertexBufferLayout core::render::Vertex::GetWgpuVertexLayout() {
    static const std::array<wgpu::VertexAttribute, 4> attribute = {

        wgpu::VertexAttribute{.format = wgpu::VertexFormat::Float32x3,
                              .offset = offsetof(Vertex, position),
                              .shaderLocation = ShaderLoc::Position},
        wgpu::VertexAttribute{.format = wgpu::VertexFormat::Float32x3,
                              .offset = offsetof(Vertex, normal),
                              .shaderLocation = ShaderLoc::Normal},
        wgpu::VertexAttribute{.format = wgpu::VertexFormat::Float32x2,
                              .offset = offsetof(Vertex, uv),
                              .shaderLocation = ShaderLoc::UV},
        wgpu::VertexAttribute{.format = wgpu::VertexFormat::Float32x4,
                              .offset = offsetof(Vertex, tangent),
                              .shaderLocation = ShaderLoc::Tangent},
    };

    return wgpu::VertexBufferLayout{
        .stepMode = wgpu::VertexStepMode::Vertex,
        .arrayStride = sizeof(Vertex),
        .attributeCount = attribute.size(),
        .attributes = attribute.data(),
    };
}

wgpu::VertexBufferLayout core::render::MapVertexDesc(VertexType type) {
    switch (type) {
        case VertexType::StandardMesh:
            return Vertex::GetWgpuVertexLayout();
        case VertexType::None:
            return {};
    }
}

}  // namespace core::render