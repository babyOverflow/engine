#pragma once

struct EmptyData {};

struct SurfaceAttributes {
    float3 baseColor;
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
                                                       float2 uv);
}

struct EmptyMaterial : IMaterial {
    typealias DataType = EmptyData;
    static SurfaceAttributes evaluateSurfaceAttributes(DataType data,
                                                       float3 worldPosition,
                                                       float3 worldNormal,
                                                       float2 uv) {
        SurfaceAttributes attributes;
        attributes.baseColor = float3(1.0, 1.0, 1.0);
        attributes.worldNormal = worldNormal;
        attributes.roughness = 0.5;
        attributes.metallic = 0.0;
        attributes.emissive = float3(0.0, 0.0, 0.0);
        return attributes;
    }
}

interface IRenderPass {
    associatedtype M: IMaterial;

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

