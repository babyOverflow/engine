#include <ShaderInterop.h>
#include <core_interface.slang>

struct EmptyMaterial : IMaterial {
    typealias DataType = EmptyData;
    static SurfaceAttributes evaluateSurfaceAttributes(DataType data,
                                                       EngineSamplers samplers,
                                                       float3 worldPosition,
                                                       float3 worldNormal,
                                                       float2 uv,
                                                       float3 tangent = 0,
                                                       float3 bitangent = 0) {
        SurfaceAttributes att;
        return att;
    }
};

float3 ComputePBR(float3 albedo,
                  float3 N,
                  float3 V,
                  float3 L,
                  float metallic,
                  float roughness,
                  float3 lightColor) {
    float3 H = normalize(V + L);
    float dotNH = max(dot(N, H), 0.0);
    float dotNV = max(dot(N, V), 0.0);
    float dotNL = max(dot(N, L), 0.0);

    // 1. Fresnel (Schlick)
    float3 F0 = lerp(float3(0.04), albedo, metallic);
    float3 F = F0 + (1.0 - F0) * pow(clamp(1.0 - max(dot(H, V), 0.0), 0.0, 1.0), 5.0);

    // 2. Normal Distribution Function (GGX)
    float a = roughness * roughness;
    float a2 = a * a;
    float denom = (dotNH * dotNH * (a2 - 1.0) + 1.0);
    float D = a2 / (3.141592 * denom * denom);

    // 3. Geometry Function (Schlick-GGX)
    float k = (roughness + 1.0) * (roughness + 1.0) / 8.0;
    float G1_V = dotNV / (dotNV * (1.0 - k) + k);
    float G1_L = dotNL / (dotNL * (1.0 - k) + k);
    float G = G1_V * G1_L;

    // BRDF 연산
    float3 nominator = D * G * F;
    float denominator = 4.0 * dotNV * dotNL + 0.0001;
    float3 specular = nominator / denominator;

    float3 kS = F;
    float3 kD = (float3(1.0) - kS) * (1.0 - metallic);

    return (kD * albedo / 3.141592 + specular) * lightColor * dotNL;
}

float3 ReconstructWorldPos(float2 uv, float depth, float4x4 invViewProj) {
    float4 clipPos = float4(uv.x * 2.0 - 1.0, (1.0 - uv.y) * 2.0 - 1.0, depth, 1.0);
    float4 worldPos = mul(invViewProj, clipPos);
    return worldPos.xyz / worldPos.w;
}

struct Vertex {
    uint vertexID : SV_VertexID;
};

struct DeferredGBuffer {
    Texture2D deferredGBufferAlbedo;
    Texture2D deferredGBufferMaterial;
    Texture2D deferredGBufferNormal;
    DepthTexture2D depthStencil;
};

struct Fragment {
    float4 color;
};

struct Varyings {
    float2 uv;
}

struct VertexStageOutput {
    Varyings coarseVertex;
    float4 sv_position : SV_Position;
};

struct DeferredLightingPass<M : IMaterial> : IRenderPass {
    typealias Material = EmptyMaterial;

    typealias VertexInputType = Vertex;
    typealias VertexOutputType = VertexStageOutput;
    typealias FragmentInputType = Varyings;
    typealias FragmentOutputType = Fragment;

    typealias GlobalResourcesType = GlobalGroup;
    typealias PassResourceType = DeferredGBuffer;

    static VertexOutputType vertex(GlobalResourcesType globalResources,
                                   Material.DataType material,
                                   PassResourceType passResources,
                                   VertexInputType input) {
        VertexStageOutput output;

        output.coarseVertex.uv = float2((input.vertexID << 1) & 2, input.vertexID & 2);
        output.sv_position =
            float4(output.coarseVertex.uv * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);

        return output;
    }

    static FragmentOutputType fragment(GlobalResourcesType globalResources,
                                       Material.DataType material,
                                       PassResourceType passResources,
                                       FragmentInputType input) {
        float4 albedo = passResources.deferredGBufferAlbedo.SampleLevel(
            globalResources.samplers.pointClamp, input.uv, 0);
        float4 normal = passResources.deferredGBufferNormal.SampleLevel(
            globalResources.samplers.pointClamp, input.uv, 0);
        float4 mat = passResources.deferredGBufferMaterial.SampleLevel(
            globalResources.samplers.pointClamp, input.uv, 0);

        float depth =
            passResources.depthStencil.Sample(globalResources.samplers.pointClamp, input.uv);

        float metallic = mat.r;
        float roughness = mat.g;
        float ao = mat.b;

        float3 worldPos = ReconstructWorldPos(input.uv, depth, globalResources.vars.invViewProj);
        float3 V = normalize(globalResources.vars.position.xyz - worldPos);
        float3 N = normalize(normal.xyz);
        float3 L = normalize(float3(-1, 1, -1));

        float3 directLight = ComputePBR(albedo.xyz, N, V, L, metallic, roughness, float3(1, 1, 1));

        float3 ambient = float3(0.4) * albedo.xyz * ao;

        float3 finalColor = ambient + directLight;

        Fragment output;
        output.color = float4(finalColor, 1.0);
        return output;
    }
}
