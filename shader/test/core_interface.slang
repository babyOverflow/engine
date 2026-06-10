#pragma once

struct EmptyData {};

struct SurfaceAttributes {
    float4 baseColor;
    float3 worldNormal;
    float roughness;
    float metallic;
    float3 emissive;
};
interface IMaterial {
    associatedtype DataType;

    static SurfaceAttributes evaluateSurfaceAttributes(DataType data,
                                                       float3 worldPosition,
                                                       float3 worldNormal,
                                                       float2 uv,
                                                       float3 tangent = 0,
                                                       float3 bitangent = 0);
}

interface IRenderPass {
    associatedtype M : IMaterial;

    associatedtype VertexInputType;
    associatedtype VertexOutputType;
    associatedtype FragmentInputType;
    associatedtype FragmentOutputType;
    associatedtype GlobalResourcesType;
    associatedtype PassResourceType;

    static VertexOutputType vertex(GlobalResourcesType globalResources,
                                   M.DataType material,
                                   PassResourceType passResources,
                                   VertexInputType input);
    static FragmentOutputType fragment(GlobalResourcesType globalResources,
                                       M.DataType material,
                                       PassResourceType passResources,
                                       FragmentInputType input);
}

struct GlobalResources {
    float4x4 viewProjMatrix;
    float3 cameraPosition;
};

