#include <ShaderInterop.h>

struct VertexStardard {
    [[vk::location(ShaderLoc::Position)]]
    float3 position;
    [[vk::location(ShaderLoc::Normal)]]
    float3 normal;
    [[vk::location(ShaderLoc::UV)]]
    float2 uv;
    [[vk::location(ShaderLoc::Tangent)]]
    float4 tangent;
};