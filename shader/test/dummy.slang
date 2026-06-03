#include "core_interface.slang"
// dummy.slang

// 1. The core interface defining flexible output and resource handling

struct MaterialA : IMaterial {
    typealias DataType = struct {
        Texture2D shadowMap;
        SamplerState baseColorSampler;
    };

    static SurfaceAttributes evaluateSurfaceAttributes(DataType data,
                                                       float3 worldPosition,
                                                       float3 worldNormal,
                                                       float2 uv) {
        SurfaceAttributes attributes;
        attributes.baseColor = data.shadowMap.Sample(data.baseColorSampler, uv).rgb;
        return attributes;
    }
};

struct DifuseMaterial : IMaterial {
    typealias DataType = struct {
        Texture2D diffuseMap;
        SamplerState linearSampler;
    };
    static SurfaceAttributes evaluateSurfaceAttributes(DataType data,
                                                       float3 worldPosition,
                                                       float3 worldNormal,
                                                       float2 uv) {
        SurfaceAttributes attributes;
        attributes.baseColor = data.diffuseMap.Sample(data.linearSampler, uv).rgb;
        return attributes;
    }
};

// ==========================================
// G-BUFFER PASS IMPLEMENTATION
// ==========================================
struct GBufferPass<Material : IMaterial> : IRenderPass {
    typealias M = Material;
    typealias VertexInputType = struct {
        float3 position : POSITION;
        float3 normal : NORMAL;
        float2 uv : TEXCOORD;
    };
    typealias VertexOutputType = struct {
        float4 position : SV_Position;
        float3 worldPos : TEXCOORD0;
        float3 normal : TEXCOORD1;
        float2 uv : TEXCOORD2;
    };
    typealias FragmentInputType = struct {
        float3 worldPos : TEXCOORD0;
        float3 normal : TEXCOORD1;
        float2 uv : TEXCOORD2;
    };
    typealias FragmentOutputType = struct GBufferColors {
        float4 albedo : SV_Target0;
        float4 normal : SV_Target1;
        float4 linearDepth : SV_Target2;
    };
    typealias GlobalResourcesType = GlobalResources;
    typealias PassResourceType = EmptyData;

    static VertexOutputType vertex(GlobalResourcesType globalResources,
                                   M.DataType material,
                                   PassResourceType passResources,
                                   VertexInputType input) {
        VertexOutputType output;
        float4 worldPos = float4(input.position, 1.0);
        output.position = mul(worldPos, globalResources.viewProjMatrix);
        output.worldPos = input.position;
        output.normal = input.normal;
        output.uv = input.uv;
        return output;
    }

    static FragmentOutputType fragment(GlobalResourcesType globalResources,
                                       M.DataType material,
                                       PassResourceType passResources,
                                       FragmentInputType input) {
        GBufferColors outTargets;
        SurfaceAttributes surfaceAttrs =
            M.evaluateSurfaceAttributes(material, input.worldPos, input.normal, input.uv);
        outTargets.albedo = float4(surfaceAttrs.baseColor, 1.0);
        outTargets.normal = float4(input.normal * 0.5 + 0.5, 1.0);
        outTargets.linearDepth = float4(input.worldPos.z, 0.0, 0.0, 1.0);
        return outTargets;
    }
}

// ==========================================
// FORWARD LIGHTING PASS IMPLEMENTATION
// ==========================================

struct ForwardPass<Material : IMaterial> : IRenderPass {
    typealias M = Material;

    typealias VertexInputType = struct {
        float3 position : POSITION;
        float3 normal : NORMAL;
        float2 uv : TEXCOORD;
    };
    typealias VertexOutputType = struct {
        float4 position : SV_Position;
        float3 worldPos : TEXCOORD0;
        float3 normal : TEXCOORD1;
        float2 uv : TEXCOORD2;
    };
    typealias FragmentInputType = struct {
        float3 worldPos : TEXCOORD0;
        float3 normal : TEXCOORD1;
        float2 uv : TEXCOORD2;
    };
    typealias FragmentOutputType = struct ForwardColor {
        float4 sceneColor : SV_Target0;
    };

    typealias GlobalResourcesType = GlobalResources;
    typealias MaterialType = MaterialA;

    // Different pass, completely different resource requirements
    struct PassResourceType {
        float4 lightDirection;
    };

    static VertexOutputType vertex(GlobalResourcesType globalResources,
                                   M.DataType material,
                                   PassResourceType passResources,
                                   VertexInputType input) {
        VertexOutputType output;
        float4 worldPos = float4(input.position, 1.0);
        output.position = mul(worldPos, globalResources.viewProjMatrix);
        output.worldPos = input.position;
        output.normal = input.normal;
        output.uv = input.uv;
        return output;
    }

    static FragmentOutputType fragment(GlobalResourcesType globalResources,
                                       M.DataType material,
                                       PassResourceType passResources,
                                       FragmentInputType input) {
        ForwardColor outTarget;
        // Compute lighting using shadowMap and lightData...
        float3 baseColor =
            M.evaluateSurfaceAttributes(material, input.worldPos, input.normal, input.uv).baseColor;
        outTarget.sceneColor = float4(baseColor, 1.0);
        return outTarget;
    }
}

// ==========================================
// GENERIC ENTRY POINT
// ==========================================
typealias P = GBufferPass<DifuseMaterial>;

ParameterBlock<P.GlobalResourcesType> globalResources;
ParameterBlock<P.M.DataType> material;
ParameterBlock<P.PassResourceType> currentPass;

[shader("vertex")]
P.VertexOutputType vertexMain(P.VertexInputType input) {
    return P.vertex(globalResources, material, currentPass, input);
}

[shader("fragment")]
P.FragmentOutputType fragmentMain(P.FragmentInputType input) {
    return P.fragment(globalResources, material, currentPass, input);
}
