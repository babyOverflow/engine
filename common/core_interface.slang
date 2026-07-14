#pragma once
#include "ShaderInterop.h"

struct EmptyData {};

struct EngineSamplers {
    SamplerState linearRepeat;
    SamplerState pointClamp;
};

struct GlobalGroup {
    GlobalUniforms vars;
    EngineSamplers samplers;
};

struct SurfaceAttributes {
    float4 baseColor;
    float3 worldNormal;
    float occlusion = 1.0;
    float roughness;
    float metallic;
    float3 emissive;
};
interface IMaterial {
    associatedtype DataType;

    static SurfaceAttributes evaluateSurfaceAttributes(DataType data,
                                                       EngineSamplers samplers,
                                                       float3 worldPosition,
                                                       float3 worldNormal,
                                                       float2 uv,
                                                       float3 tangent = 0,
                                                       float3 bitangent = 0);
}

interface IRenderPass {
    associatedtype Material : IMaterial;

    associatedtype VertexInputType;
    associatedtype VertexOutputType;
    associatedtype FragmentInputType;
    associatedtype FragmentOutputType;
    associatedtype GlobalResourcesType;
    associatedtype PassResourceType;

    static VertexOutputType vertex(GlobalResourcesType globalResources,
                                   Material.DataType material,
                                   PassResourceType passResources,
                                   VertexInputType input);
    static FragmentOutputType fragment(GlobalResourcesType globalResources,
                                       Material.DataType material,
                                       PassResourceType passResources,
                                       FragmentInputType input);
}
