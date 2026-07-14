#include <ShaderInterop.h>
#include <core_interface.slang>

struct OpaqueLitMaterial : IMaterial {
    typealias DataType = struct {
        Texture2D<float4> baseColorTexture;
        Texture2D<float4> metallicRoughnessTexture;
        Texture2D<float4> normalTexture;
    };
    static SurfaceAttributes evaluateSurfaceAttributes(DataType data,
                                                       EngineSamplers samplers,
                                                       float3 worldPosition,
                                                       float3 normal,
                                                       float2 uv,
                                                       float3 tangent = 0,
                                                       float3 bitangent = 0) {
        SurfaceAttributes attributes;

        float3 lightDir = normalize(float3(1.0f, 1.0f, 0.5f));

        float3 T = normalize(tangent);
        float3 B = normalize(bitangent);
        float3 N = normalize(normal);

        float3x3 TBN = float3x3(T, B, N);
        float3 tangentNormal =
            data.normalTexture.Sample(samplers.linearRepeat, uv).xyz * 2.0f - 1.0f;
        float3 worldNormal = normalize(mul(tangentNormal, TBN)); 
        float4 metallicRoughness = data.metallicRoughnessTexture.Sample(samplers.linearRepeat, uv);

        attributes.baseColor = data.baseColorTexture.Sample(samplers.linearRepeat, uv);
        attributes.worldNormal = worldNormal;
        attributes.occlusion = metallicRoughness.x;
        attributes.roughness = metallicRoughness.y;
        attributes.metallic = metallicRoughness.z;
        return attributes;
    }
};

struct AssembledVertex {
    [[vk::location(0)]]
    float3 position : POSITION;
    [[vk::location(1)]]
    float3 normal : NORMAL;
    [[vk::location(2)]]
    float2 texcoord : TEXCOORD0;
    [[vk::location(3)]]
    float4 tangent : TANGENT;
};

struct Varyings {
    float3 worldPos;
    float3 normal;
    float2 uv;
    float3 tangent;
    float3 bitangent;
};

struct VertexStageOutput {
    Varyings coarseVertex : CoarseVertex;
    float4 sv_position : SV_Position;
};

struct GBufferOutput {
    float4 deferredGBufferAlbedo : SV_Target0;
    float4 deferredGBufferMaterial : SV_Target1;
    float4 deferredGBufferNormal : SV_Target2;
};
struct DeferredGBufferPass<M : IMaterial> : IRenderPass {
    typealias Material = M;
    typealias VertexInputType = AssembledVertex;
    typealias VertexOutputType = VertexStageOutput;
    typealias FragmentInputType = Varyings;
    typealias FragmentOutputType = GBufferOutput;
    typealias GlobalResourcesType = GlobalGroup;
    typealias PassResourceType = EmptyData;

    static VertexOutputType vertex(GlobalResourcesType globalResources,
                                   Material.DataType material,
                                   PassResourceType passResources,
                                   VertexInputType input) {
        VertexOutputType output;

        float3 position = input.position;
        float3 normal = normalize(input.normal);
        float3 tangent = normalize(input.tangent.xyz);
        float3 bitangent = normalize(cross(normal, tangent)) * input.tangent.w;

        output.coarseVertex.worldPos = position;
        output.coarseVertex.uv = input.texcoord;
        output.coarseVertex.normal = normal;
        output.coarseVertex.tangent = tangent;
        output.coarseVertex.bitangent = bitangent;
        output.sv_position = mul(globalResources.vars.viewProj, float4(position, 1.0f));

        return output;
    }

    static FragmentOutputType fragment(GlobalResourcesType globalResources,
                                       Material.DataType material,
                                       PassResourceType passResources,
                                       FragmentInputType input) {
        GBufferOutput output;

        SurfaceAttributes attribute = Material.evaluateSurfaceAttributes(
            material, globalResources.samplers, input.worldPos, input.normal, input.uv,
            input.tangent, input.bitangent);
        output.deferredGBufferAlbedo = attribute.baseColor;
        output.deferredGBufferMaterial = float4(attribute.occlusion, attribute.roughness, attribute.metallic, 0.0);
        output.deferredGBufferNormal.xyz = attribute.worldNormal;

        return output;
    }
};
