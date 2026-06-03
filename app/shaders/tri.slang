#include <ShaderInterop.h>

[[vk::binding(0, BindSlot::Global)]]
ConstantBuffer<GlobalUniforms> globalUniforms;


// [[vk::binding(0, BindSlot::Instance)]]
// ConstantBuffer<InstanceUniforms> instanceUniforms;

// Per-vertex attributes to be assembled from bound vertex buffers.
struct AssembledVertex {
    [[vk::location(ShaderLoc::Position)]]
    float3 position : POSITION;
    [[vk::location(ShaderLoc::Normal)]]
    float3 normal : NORMAL;
    [[vk::location(ShaderLoc::UV)]]
    float2 texcoord : TEXCOORD0;
    [[vk::location(ShaderLoc::Tangent)]]
    float4 tangent : TANGENT;
};

// Output of the vertex shader, and input to the fragment shader.

// Output of the fragment shader
struct Fragment {
    float4 color;
};

struct ColorVertex {
    float3 position : POSITION;
    float4 color : COLOR0;
};

struct ColorVertexStageOutput {
    float4 sv_position : SV_Position;
    float4 color : COLOR;
};

[shader("vertex")]
ColorVertexStageOutput vertexMain_Color(ColorVertex colorVertex) {
    ColorVertexStageOutput output;

    float3 position = colorVertex.position;

    output.sv_position = mul(globalUniforms.viewProj, float4(position, 1.0f));
    output.color = colorVertex.color;

    return output;
}

[shader("fragment")]
Fragment fragmentMain_Color(float4 color: COLOR) : SV_Target {
    Fragment output;
    output.color = color;
    return output;
}

struct Varyings {
    float3 normal;
    float2 uv;
    float3 tangent;
    float3 bitangent;
};

// Vertex  Shader
struct VertexStageOutput {
    Varyings coarseVertex : CoarseVertex;
    float4 sv_position : SV_Position;
};

[shader("vertex")]
VertexStageOutput vertexMain(AssembledVertex assembledVertex) {
    VertexStageOutput output;

    float3 position = assembledVertex.position;
    float3 normal = normalize(assembledVertex.normal);
    float3 tangent = normalize(assembledVertex.tangent.xyz);
    float3 bitangent = normalize(cross(normal, tangent)) * assembledVertex.tangent.w;

    output.coarseVertex.uv = assembledVertex.texcoord;
    output.coarseVertex.normal = normal;
    output.coarseVertex.tangent = tangent;
    output.coarseVertex.bitangent = bitangent;

    // float4 worldPosition = mul(instanceUniforms.model, float4(position, 1.0f));
    output.sv_position = mul(globalUniforms.viewProj, float4(position, 1.0f));

    return output;
}

// Fragment Shader
struct MaterialData {
    SamplerState linearSampler;
    Texture2D<float4> baseColorTexture;
    Texture2D<float4> metallicRoughnessTexture;
    Texture2D<float4> normalTexture;
};
[[vk::binding(0, BindSlot::Material)]]
ParameterBlock<MaterialData> materialData;


[shader("fragment")]
Fragment fragmentMain(Varyings coarseVertex: CoarseVertex) : SV_Target {
    float3 lightDir = normalize(float3(1.0f, 1.0f, 0.5f));
    Fragment output;

    float3 T = normalize(coarseVertex.tangent);
    float3 B = normalize(coarseVertex.bitangent);
    float3 N = normalize(coarseVertex.normal);
    float3x3 TBN = float3x3(T, B, N);

    float3 tangentNormal =
        materialData.normalTexture.Sample(materialData.linearSampler, coarseVertex.uv).xyz * 2.0f -
        1.0f;
    float3 worldNormal = normalize(mul(tangentNormal, TBN));

    float3 diffuse = max(dot(worldNormal, lightDir), 0.0f) * float3(1.0f, 1.0f, 1.0f);
    output.color = float4(diffuse, 1) * materialData.baseColorTexture.Sample(
                                            materialData.linearSampler, coarseVertex.uv);
    return output;
}

struct GBufferFragment {
    float4 albedo;
    float4 normal;
    float4 metallicRoughness;
};
[shader("fragment")]
GBufferFragment gbufferFragmentMain(Varyings coarseVertex: CoarseVertex) : SV_Target {
    GBufferFragment output;
    float3 T = normalize(coarseVertex.tangent);
    float3 B = normalize(coarseVertex.bitangent);
    float3 N = normalize(coarseVertex.normal);
    float3x3 TBN = float3x3(T, B, N);

    float3 tangentNormal =
        materialData.normalTexture.Sample(materialData.linearSampler, coarseVertex.uv).xyz * 2.0f -
        1.0f;
    float3 worldNormal = normalize(mul(tangentNormal, TBN));

    output.albedo =
        materialData.baseColorTexture.Sample(materialData.linearSampler, coarseVertex.uv);
    output.normal = float4(worldNormal, 1.0f);
    output.metallicRoughness =
        materialData.metallicRoughnessTexture.Sample(materialData.linearSampler, coarseVertex.uv);
    return output;
}

struct VertexLightingOutput {
    float4 pos : SV_Position;
    float2 uv : TexCoord;
};

[shader("vertex")]
VertexLightingOutput vertexLightingMain(uint vID: SV_VertexID) {
    VertexLightingOutput output;

    output.uv = float2((vID << 1) & 2, vID & 2);

    output.pos = float4(output.uv * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);

    return output;
}

struct GBuffer {
    SamplerState NearSampler;
    Texture2D<float4> baseColor;
    Texture2D<float4> metallicRoughness;
    Texture2D<float4> normal;
    DepthTexture2D depth;
};
[[vk::binding(0, BindSlot::Pass)]]
ParameterBlock<GBuffer> gBuffer;

[shader("fragment")]
Fragment fragmentLightingMain(float2 uv: TexCoord) : SV_Target {
    Fragment output;
    float3 lightDir = normalize(float3(1.0f, 1.0f, 0.5f));

    return output;
}
