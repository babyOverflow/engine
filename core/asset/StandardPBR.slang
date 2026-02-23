#include <ShaderInterop.h>

[[vk::binding(0, BindSlot::Global)]]
ConstantBuffer<GlobalUniforms> globalUniforms;

struct MaterialData {
    Texture2D<float4> baseColorTexture;
    SamplerState baseColorSampler;
};
[[vk::binding(0, BindSlot::Material)]]
ParameterBlock<MaterialData> materialData;

// [[vk::binding(0, BindSlot::Instance)]]
// ConstantBuffer<InstanceUniforms> instanceUniforms;

// Per-vertex attributes to be assembled from bound vertex buffers.
struct AssembledVertex {
    [[vk::location(ShaderLoc::Position)]]
    float3 position : POSITION;
    [[vk::location(ShaderLoc::Normal)]]
    float3 normal : NORMAL;
    [[vk::location(ShaderLoc::UV)]]
    float2 texcoord : TEXCOORD_0;
    [[vk::location(ShaderLoc::Tangent)]]
    float4 tangent : TANGENT;
};

// Output of the vertex shader, and input to the fragment shader.
struct CoarseVertex {
    float2 uv;
};

// Output of the fragment shader
struct Fragment {
    float4 color;
};

// Vertex  Shader
struct VertexStageOutput {
    CoarseVertex coarseVertex : CoarseVertex;
    float4 sv_position : SV_Position;
};

[shader("vertex")]
VertexStageOutput vertexMain(AssembledVertex assembledVertex) {
    VertexStageOutput output;

    float3 position = assembledVertex.position;
    float3 color = assembledVertex.normal;

    output.coarseVertex.uv = assembledVertex.texcoord;

    //float4 worldPosition = mul(instanceUniforms.model, float4(position, 1.0f));
    output.sv_position = mul(globalUniforms.viewProj, float4(position, 1.0f));

    return output;
}

// Fragment Shader
[shader("fragment")]
Fragment fragmentMain(CoarseVertex coarseVertex: CoarseVertex) : SV_Target {
    Fragment output;
    output.color =
        materialData.baseColorTexture.Sample(materialData.baseColorSampler, coarseVertex.uv);
    return output;
}
