struct SLANG_ParameterGroup_ShaderToyUniforms_std140_0
{
    @align(16) iMouse_0 : vec4<f32>,
    @align(16) iResolution_0 : vec2<f32>,
    @align(8) iTime_0 : f32,
};

@binding(0) @group(0) var<uniform> ShaderToyUniforms_0 : SLANG_ParameterGroup_ShaderToyUniforms_std140_0;
struct pixelOutput_0
{
    @location(0) output_0 : vec4<f32>,
};

fn fract_0( value_0 : f32) -> f32
{
    return fract(value_0);
}

fn ExampleEffect_rand_0( n_0 : f32) -> f32
{
    return fract_0(sin(n_0) * 43758.546875f);
}

fn mix_0( a_0 : f32,  b_0 : f32,  t_0 : f32) -> f32
{
    return mix(a_0, b_0, t_0);
}

fn ExampleEffect_mainImage_0( fragColor_0 : ptr<function, vec4<f32>>,  fragCoord_0 : vec2<f32>)
{
    var pos_0 : vec2<f32> = (fragCoord_0 / vec2<f32>(length(ShaderToyUniforms_0.iResolution_0.xy)) + vec2<f32>(ShaderToyUniforms_0.iTime_0) * vec2<f32>(0.25f, 0.0f)) * vec2<f32>(5.0f);
    var center_0 : vec2<f32> = floor(pos_0 + vec2<f32>(0.5f));
    var _S1 : f32 = center_0.x;
    var _S2 : f32 = center_0.y;
    var _S3 : f32 = ExampleEffect_rand_0(_S1 * 3.0f + _S2 * 7.0f);
    var _S4 : f32 = ExampleEffect_rand_0(_S1 * 7.0f + _S2 * 13.0f);
    var _S5 : f32 = ExampleEffect_rand_0(_S1 * 13.0f + _S2 * 3.0f);
    var radius_0 : f32 = 0.5f * mix_0(mix_0(0.10000000149011612f, 0.40000000596046448f, _S5), mix_0(0.20000000298023224f, 0.89999997615814209f, _S3), 0.5f * (1.0f + cos(ShaderToyUniforms_0.iTime_0 * mix_0(5.0f, 8.0f, _S4) + mix_0(0.0f, 4.0f, _S3))));
    var distance_0 : f32 = length(pos_0 - center_0);
    var _S6 : vec3<f32> = vec3<f32>(_S3, _S4, _S5);
    (*fragColor_0).x = _S6.x;
    (*fragColor_0).y = _S6.y;
    (*fragColor_0).z = _S6.z;
    (*fragColor_0)[i32(3)] = 1.0f;
    if(distance_0 > radius_0)
    {
        var _S7 : vec3<f32> = vec3<f32>(0.25f);
        (*fragColor_0).x = _S7.x;
        (*fragColor_0).y = _S7.y;
        (*fragColor_0).z = _S7.z;
    }
    return;
}

@fragment
fn fragmentMain(@builtin(position) sv_position_0 : vec4<f32>) -> pixelOutput_0
{
    var fragCoord_1 : vec2<f32> = sv_position_0.xy;
    var fragColor_1 : vec4<f32> = vec4<f32>(vec4<i32>(i32(0)));
    ExampleEffect_mainImage_0(&(fragColor_1), fragCoord_1);
    var _S8 : pixelOutput_0 = pixelOutput_0( vec4<f32>(fragColor_1.xyz, 1.0f) );
    return _S8;
}

