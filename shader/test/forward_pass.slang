#include <ShaderInterop.h>
#include <core_interface.slang>

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
    float3 normal;
    float2 uv;
    float3 tangent;
    float3 bitangent;
};

struct VertexStageOutput {
    Varyings coarseVertex : CoarseVertex;
    float4 sv_position : SV_Position;
};

struct Fragment {
    float4 color;
};

struct ForwardMaterial : IMaterial {
    typealias DataType = struct {
        SamplerState linearSampler;
        Texture2D<float4> baseColorTexture;
        Texture2D<float4> metallicRoughnessTexture;
        Texture2D<float4> normalTexture;
    };
    static SurfaceAttributes evaluateSurfaceAttributes(DataType data,
                                                       float3 worldPosition,
                                                       float3 normal,
                                                       float2 uv,
                                                       float3 tangent = 0,
                                                       float3 bitangent = 0) {
        float3 lightDir = normalize(float3(1.0f, 1.0f, 0.5f));

        float3 T = normalize(tangent);
        float3 B = normalize(bitangent);
        float3 N = normalize(normal);

        float3x3 TBN = float3x3(T, B, N);
        float3 tangentNormal = data.normalTexture.Sample(data.linearSampler, uv).xyz * 2.0f - 1.0f;
        float3 worldNormal = normalize(mul(tangentNormal, TBN));

        SurfaceAttributes attributes;
        float3 diffuse = max(dot(worldNormal, lightDir), 0.0f) * float3(1.0f, 1.0f, 1.0f);
        attributes.baseColor =
            float4(diffuse, 1) * data.baseColorTexture.Sample(data.linearSampler, uv);
        return attributes;
    }
};

struct ForwardRenderPass<Material : IMaterial> : IRenderPass {
    typealias M = Material;
    typealias VertexInputType = AssembledVertex;
    typealias VertexOutputType = VertexStageOutput;
    typealias FragmentInputType = Varyings;
    typealias FragmentOutputType = Fragment;
    typealias GlobalResourcesType = GlobalResources;
    typealias PassResourceType = EmptyData;
    static VertexOutputType vertex(GlobalResourcesType globalResources,
                                   M.DataType material,
                                   PassResourceType passResources,
                                   VertexInputType input) {
        VertexOutputType output;

        float3 position = input.position;
        float3 normal = normalize(input.normal);
        float3 tangent = normalize(input.tangent.xyz);
        float3 bitangent = normalize(cross(normal, tangent)) * input.tangent.w;

        output.coarseVertex.uv = input.texcoord;
        output.coarseVertex.normal = normal;
        output.coarseVertex.tangent = tangent;
        output.coarseVertex.bitangent = bitangent;
        output.sv_position = mul(globalResources.viewProjMatrix, float4(position, 1.0f));

        return output;
    }

    static FragmentOutputType fragment(GlobalResourcesType globalResources,
                                       M.DataType material,
                                       PassResourceType passResources,
                                       FragmentInputType input) {
        SurfaceAttributes attributes =
            M.evaluateSurfaceAttributes(material, float3(0.0, 0.0, 0.0), input.normal, input.uv,
                                        input.tangent, input.bitangent);

        FragmentOutputType output;
        output.color = attributes.baseColor;
        return output;
    }
};
