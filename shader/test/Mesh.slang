module Mesh;

#include <ShaderInterop.h>

public struct AssembledVertex {
    [[vk::location(ShaderLoc::Position)]]
    public float3 position;
    [[vk::location(ShaderLoc::Normal)]]
    public float3 normal;
    [[vk::location(ShaderLoc::UV)]]
    public float2 uv;
    [[vk::location(ShaderLoc::Tangent)]]
    public float4 tangent;
};
