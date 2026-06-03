@binding(0) @group(0) var entryPointParams_currentPass_diffuseMap_0 : texture_2d<f32>;

@binding(1) @group(0) var entryPointParams_currentPass_linearSampler_0 : sampler;

struct GBufferColors_0
{
    @location(0) albedo_0 : vec4<f32>,
    @location(1) normal_0 : vec4<f32>,
    @location(2) linearDepth_0 : vec4<f32>,
};

struct pixelInput_0
{
    @location(0) worldPos_0 : vec3<f32>,
    @location(1) worldNormal_0 : vec3<f32>,
    @location(2) uv_0 : vec2<f32>,
};

fn GBufferPass_execute_0( this_diffuseMap_0 : texture_2d<f32>,  this_linearSampler_0 : sampler,  worldPos_1 : vec3<f32>,  normal_1 : vec3<f32>,  uv_1 : vec2<f32>) -> GBufferColors_0
{
    var outTargets_0 : GBufferColors_0;
    outTargets_0.albedo_0 = (textureSample((this_diffuseMap_0), (this_linearSampler_0), (uv_1)));
    var _S1 : vec3<f32> = vec3<f32>(0.5f);
    outTargets_0.normal_0 = vec4<f32>(normal_1 * _S1 + _S1, 1.0f);
    outTargets_0.linearDepth_0 = vec4<f32>(worldPos_1.z, 0.0f, 0.0f, 1.0f);
    return outTargets_0;
}

@fragment
fn fragmentMain( _S2 : pixelInput_0, @builtin(position) screenPos_0 : vec4<f32>) -> GBufferColors_0
{
    return GBufferPass_execute_0(entryPointParams_currentPass_diffuseMap_0, entryPointParams_currentPass_linearSampler_0, _S2.worldPos_0, normalize(_S2.worldNormal_0), _S2.uv_0);
}

