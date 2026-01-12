#pragma once

#ifdef __cplusplus
#include <cstdint>
#include <glm/glm.hpp>  // GLM »ç¿ë ½Ã

using uint = uint32_t;
using float2 = glm::vec2;
using float3 = glm::vec3;
using float4 = glm::vec4;
using float4x4 = glm::mat4;

#define NAMESPACE_BEGIN(name) namespace name {
#define NAMESPACE_END }
#elif defined(__SLANG__)

#define NAMESPACE_BEGIN(name)
#define NAMESPACE_END

#else
#error "Unknown compiler/language! This header supports only C++ and Slang."
#endif


struct ShaderLoc {
    static const uint Position = 0;
    static const uint Normal = 1;
    static const uint UV = 2;
    static const uint Tangent = 3;
    static const uint Color = 4;
};

struct BindSlot {
    static const uint Global = 0;
};

