#pragma pack_matrix(column_major)
#ifdef SLANG_HLSL_ENABLE_NVAPI
#include "nvHLSLExtns.h"
#endif

#ifndef __DXC_VERSION_MAJOR
// warning X3557: loop doesn't seem to do anything, forcing loop to unroll
#pragma warning(disable : 3557)
#endif


#line 4887 "hlsl.meta.slang"
RWTexture2D<float4 > entryPointParams_image_0 : register(u0);


#line 200 "shader-toy.slang"
struct SLANG_ParameterGroup_ShaderToyUniforms_0
{
    float4 iMouse_0;
    float2 iResolution_0;
    float iTime_0;
};


#line 200
cbuffer ShaderToyUniforms_0 : register(b0)
{
    SLANG_ParameterGroup_ShaderToyUniforms_0 ShaderToyUniforms_0;
}

#line 341
float fract_0(float value_0)
{
    return frac(value_0);
}


#line 34 "example-effect.slang"
float ExampleEffect_rand_0(float n_0)
{
    return fract_0(sin(n_0) * 43758.546875f);
}


#line 346 "shader-toy.slang"
float mix_0(float a_0, float b_0, float t_0)
{
    return lerp(a_0, b_0, t_0);
}


#line 39 "example-effect.slang"
void ExampleEffect_mainImage_0(out float4 fragColor_0, float2 fragCoord_0)
{

#line 45
    float2 pos_0 = (fragCoord_0 / length(ShaderToyUniforms_0.iResolution_0.xy) + ShaderToyUniforms_0.iTime_0 * float2(0.25f, 0.0f)) * 5.0f;

    float2 center_0 = floor(pos_0 + (float2)0.5f);

    float _S1 = center_0.x;

#line 49
    float _S2 = center_0.y;

#line 49
    float _S3 = ExampleEffect_rand_0(_S1 * 3.0f + _S2 * 7.0f);

#line 49
    float _S4 = ExampleEffect_rand_0(_S1 * 7.0f + _S2 * 13.0f);

#line 49
    float _S5 = ExampleEffect_rand_0(_S1 * 13.0f + _S2 * 3.0f);

#line 61
    float radius_0 = 0.5f * mix_0(mix_0(0.10000000149011612f, 0.40000000596046448f, _S5), mix_0(0.20000000298023224f, 0.89999997615814209f, _S3), 0.5f * (1.0f + cos(ShaderToyUniforms_0.iTime_0 * mix_0(5.0f, 8.0f, _S4) + mix_0(0.0f, 4.0f, _S3))));


    float distance_0 = length(pos_0 - center_0);

    fragColor_0.xyz = float3(_S3, _S4, _S5);
    fragColor_0[int(3)] = 1.0f;

    if(distance_0 > radius_0)
    {

#line 69
        fragColor_0.xyz = (float3)0.25f;

#line 69
    }
    return;
}


#line 163 "shader-toy.slang"
[numthreads(1, 1, 1)]
void computeMain(uint3 sv_dispatchThreadID_0 : SV_DispatchThreadID)
{

#line 173
    uint2 _S6 = sv_dispatchThreadID_0.xy;

#line 173
    float2 _S7 = float2(_S6);
    float4 fragColor_1 = float4((int4)int(0));

#line 174
    ExampleEffect_mainImage_0(fragColor_1, _S7);

#line 181
    entryPointParams_image_0[_S6] = fragColor_1;
    return;
}

