Generated WGSL Code:
struct _MatrixStorage_float4x4_ColMajorstd140_0
{
    @align(16) data_0 : array<vec4<f32>, i32(4)>,
};

struct GlobalResources_std140_0
{
    @align(16) viewProjMatrix_0 : _MatrixStorage_float4x4_ColMajorstd140_0,
    @align(16) cameraPosition_0 : vec3<f32>,
};

@binding(0) @group(0) var<uniform> globalResources_0 : GlobalResources_std140_0;
@binding(0) @group(1) var material_diffuseMap_0 : texture_2d<f32>;

@binding(1) @group(1) var material_linearSampler_0 : sampler;

struct GBufferPass_SLANG_anonymous_2_0
{
    @builtin(position) position_0 : vec4<f32>,
    @location(0) worldPos_0 : vec3<f32>,
    @location(1) normal_0 : vec3<f32>,
    @location(2) uv_0 : vec2<f32>,
};

struct GBufferPass_SLANG_anonymous_1_0
{
     position_1 : vec3<f32>,
     normal_1 : vec3<f32>,
     uv_1 : vec2<f32>,
};

fn GBufferPass_vertex_0( globalResources_1 : ptr<function, GlobalResources_std140_0>,  material_diffuseMap_1 : texture_2d<f32>,  material_linearSampler_1 : sampler,  input_0 : GBufferPass_SLANG_anonymous_1_0) -> GBufferPass_SLANG_anonymous_2_0
{
    var output_0 : GBufferPass_SLANG_anonymous_2_0;
    output_0.position_0 = (((mat4x4<f32>((*globalResources_1).viewProjMatrix_0.data_0[i32(0)][i32(0)], (*globalResources_1).viewProjMatrix_0.data_0[i32(1)][i32(0)], (*globalResources_1).viewProjMatrix_0.data_0[i32(2)][i32(0)], (*globalResources_1).viewProjMatrix_0.data_0[i32(3)][i32(0)], (*globalResources_1).viewProjMatrix_0.data_0[i32(0)][i32(1)], (*globalResources_1).viewProjMatrix_0.data_0[i32(1)][i32(1)], (*globalResources_1).viewProjMatrix_0.data_0[i32(2)][i32(1)], (*globalResources_1).viewProjMatrix_0.data_0[i32(3)][i32(1)], (*globalResources_1).viewProjMatrix_0.data_0[i32(0)][i32(2)], (*globalResources_1).viewProjMatrix_0.data_0[i32(1)][i32(2)], (*globalResources_1).viewProjMatrix_0.data_0[i32(2)][i32(2)], (*globalResources_1).viewProjMatrix_0.data_0[i32(3)][i32(2)], (*globalResources_1).viewProjMatrix_0.data_0[i32(0)][i32(3)], (*globalResources_1).viewProjMatrix_0.data_0[i32(1)][i32(3)], (*globalResources_1).viewProjMatrix_0.data_0[i32(2)][i32(3)], (*globalResources_1).viewProjMatrix_0.data_0[i32(3)][i32(3)])) * (vec4<f32>(input_0.position_1, 1.0f))));
    output_0.worldPos_0 = input_0.position_1;
    output_0.normal_0 = input_0.normal_1;
    output_0.uv_0 = input_0.uv_1;
    return output_0;
}

struct vertexInput_0
{
    @location(0) position_2 : vec3<f32>,
    @location(1) normal_2 : vec3<f32>,
    @location(2) uv_2 : vec2<f32>,
};

@vertex
fn vertexMain( _S1 : vertexInput_0) -> GBufferPass_SLANG_anonymous_2_0
{
    var _S2 : GBufferPass_SLANG_anonymous_1_0 = GBufferPass_SLANG_anonymous_1_0( _S1.position_2, _S1.normal_2, _S1.uv_2 );
    var _S3 : GlobalResources_std140_0 = globalResources_0;
    var _S4 : GBufferPass_SLANG_anonymous_2_0 = GBufferPass_vertex_0(&(_S3), material_diffuseMap_0, material_linearSampler_0, _S2);
    return _S4;
}

struct GBufferPass_GBufferColors_0
{
    @location(0) albedo_0 : vec4<f32>,
    @location(1) normal_3 : vec4<f32>,
    @location(2) linearDepth_0 : vec4<f32>,
};

struct GBufferPass_SLANG_anonymous_3_0
{
     worldPos_1 : vec3<f32>,
     normal_4 : vec3<f32>,
     uv_3 : vec2<f32>,
};

struct SurfaceAttributes_0
{
     baseColor_0 : vec3<f32>,
     worldNormal_0 : vec3<f32>,
     roughness_0 : f32,
     metallic_0 : f32,
     emissive_0 : vec3<f32>,
};

fn DiffuseMaterial_evaluateSurfaceAttributes_0( data_diffuseMap_0 : texture_2d<f32>,  data_linearSampler_0 : sampler,  worldPosition_0 : vec3<f32>,  worldNormal_1 : vec3<f32>,  uv_4 : vec2<f32>) -> SurfaceAttributes_0
{
    var attributes_0 : SurfaceAttributes_0;
    attributes_0.baseColor_0 = (textureSample((data_diffuseMap_0), (data_linearSampler_0), (uv_4))).xyz;
    return attributes_0;
}

fn GBufferPass_fragment_0( globalResources_2 : ptr<function, GlobalResources_std140_0>,  material_diffuseMap_2 : texture_2d<f32>,  material_linearSampler_2 : sampler,  input_1 : GBufferPass_SLANG_anonymous_3_0) -> GBufferPass_GBufferColors_0
{
    var outTargets_0 : GBufferPass_GBufferColors_0;
    outTargets_0.albedo_0 = vec4<f32>(DiffuseMaterial_evaluateSurfaceAttributes_0(material_diffuseMap_2, material_linearSampler_2, input_1.worldPos_1, input_1.normal_4, input_1.uv_3).baseColor_0, 1.0f);
    var _S5 : vec3<f32> = vec3<f32>(0.5f);
    outTargets_0.normal_3 = vec4<f32>(input_1.normal_4 * _S5 + _S5, 1.0f);
    outTargets_0.linearDepth_0 = vec4<f32>(input_1.worldPos_1.z, 0.0f, 0.0f, 1.0f);
    return outTargets_0;
}

struct pixelInput_0
{
    @location(0) worldPos_2 : vec3<f32>,
    @location(1) normal_5 : vec3<f32>,
    @location(2) uv_5 : vec2<f32>,
};

@fragment
fn fragmentMain( _S6 : pixelInput_0) -> GBufferPass_GBufferColors_0
{
    var _S7 : GBufferPass_SLANG_anonymous_3_0 = GBufferPass_SLANG_anonymous_3_0( _S6.worldPos_2, _S6.normal_5, _S6.uv_5 );
    var _S8 : GlobalResources_std140_0 = globalResources_0;
    var _S9 : GBufferPass_GBufferColors_0 = GBufferPass_fragment_0(&(_S8), material_diffuseMap_0, material_linearSampler_0, _S7);
    return _S9;
}
