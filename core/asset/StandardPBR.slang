#include <ShaderInterop.h>

interface IPass {
    // 반환 타입 자체를 제네릭하게 비워둡니다.
    associatedtype Output;
    [[vk::binding(0, BindSlot::Global)]]
    associatedtype GlobalGroup;
    [[vk::binding(0, BindSlot::Pass)]]
    associatedtype PassGroup;
    [[vk::binding(0, BindSlot::Material)]]
    associatedtype MaterialGroup;

    Output Run();
}

struct ForwardOutput {
    float4 color : SV_Target0;
};



// [GBuffer 패스용 멀티 타겟 출력]
struct GBufferOutput {
    float4 albedo : SV_Target0;
    float4 normal : SV_Target1;
    float4 material : SV_Target2;
};

[shader("fragment")]
T.Output fragmentMain<T : IPass>() {
    T pass;
    return pass.Run();
}

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
    float2 texcoord : TEXCOORD0;
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
