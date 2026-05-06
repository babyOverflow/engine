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

// Output of the vertex shader, and input to the fragment shader.
struct CoarseVertex {
    float2 uv;
    float3 viewDir;
    float3 reflection;
    float diffuse;
};

struct VertexStageOutput {
    CoarseVertex coarseVertex : CoarseVertex;
    float4 sv_position : SV_Position;
};

[shader("vertex")]
VertexStageOutput vertexMain(AssembledVertex assembledVertex) {
    VertexStageOutput output;

    float3 position = assembledVertex.position;
    float3 color = assembledVertex.normal;

    float3 lightDir = assembledVertex.position.xyz - float3(-4000, -4000, -4000);
    lightDir = normalize(lightDir);
    float3 worldNormal = normalize(assembledVertex.normal);
    output.coarseVertex.diffuse = max(dot(worldNormal, -lightDir), 0.0f);

    float3 viewDir = normalize(globalUniforms.position.xyz - assembledVertex.position);
    output.coarseVertex.viewDir = viewDir;

    float3 reflection = reflect(lightDir, worldNormal);
    output.coarseVertex.reflection = reflection;

    output.coarseVertex.uv = assembledVertex.texcoord;
    // float4 worldPosition = mul(instanceUniforms.model, float4(position, 1.0f));
    output.sv_position = mul(globalUniforms.viewProj, float4(position, 1.0f));

    return output;
}

struct MaterialData {
    Texture2D<float4> baseColorTexture;
    SamplerState baseColorSampler;
    Texture2D<float4> metallicRoughnessTexture;
    SamplerState metallicRoughnessTextureSampler;
};

[[vk::binding(0, BindSlot::Material)]]
ParameterBlock<MaterialData> materialData;

float RoughnessToShininess(float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    return max(2.0 / (a2 + 0.0001) - 2.0, 0.0);
}

// Fragment Shader
[shader("fragment")]
Fragment fragmentMain(CoarseVertex coarseVertex: CoarseVertex) : SV_Target {
    float4 lightColor = float4(1, 1, 1, 1);
    Fragment output;
    float diffuse = saturate(coarseVertex.diffuse);

    float3 reflection = normalize(coarseVertex.reflection);
    float3 viewDir = normalize(coarseVertex.viewDir);

    float specular = 0;
    if (diffuse > 0) {
        specular = saturate(dot(reflection, viewDir));
        specular = pow(specular, 16);
    }

    float4 baseColor =
        materialData.baseColorTexture.Sample(materialData.baseColorSampler, coarseVertex.uv);
    float4 metalRoughtness = materialData.metallicRoughnessTexture.Sample(
        materialData.metallicRoughnessTextureSampler, coarseVertex.uv);
    float occlusion = metalRoughtness.r;
    float roughness = metalRoughtness.g;
    float matallness = metalRoughtness.b;

    float4 dielectricSpec = float4(0.04, 0.04, 0.04, 1);
    float4 specColor = lerp(dielectricSpec, baseColor, matallness);
    float4 diffuseColor = baseColor * (1 - matallness);

    float shiniess = RoughnessToShininess(roughness);

    float4 diffuseComponent = diffuseColor * diffuse;

    output.color = baseColor;

    return output;
}
