#include "Mesh.h"

namespace core::render {

 const std::array<wgpu::VertexAttribute, 4> Vertex::GetAttributes() {
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

     return attribute;
 }
}  // namespace core::render