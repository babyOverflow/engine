struct _MatrixStorage_float4x4_ColMajorstd140_0
{
    @align(16) data_0 : array<vec4<f32>, i32(4)>,
};

struct SLANG_ParameterGroup_Uniforms_std140_0
{
    @align(16) modelViewProjection_0 : _MatrixStorage_float4x4_ColMajorstd140_0,
};

@binding(0) @group(0) var<uniform> Uniforms_0 : SLANG_ParameterGroup_Uniforms_std140_0;
struct VertexStageOutput_0
{
    @location(0) color_0 : vec3<f32>,
    @builtin(position) sv_position_0 : vec4<f32>,
};

struct vertexInput_0
{
    @location(0) position_0 : vec3<f32>,
    @location(1) normal_0 : vec3<f32>,
    @location(2) texcoord_0 : vec2<f32>,
    @location(3) tangent_0 : vec4<f32>,
};

struct CoarseVertex_0
{
     _S1 : vec3<f32>,
};

struct VertexStageOutput_1
{
     coarseVertex_0 : CoarseVertex_0,
     _S2 : vec4<f32>,
};

@vertex
fn vertexMain( _S3 : vertexInput_0) -> VertexStageOutput_0
{
    var output_0 : VertexStageOutput_1;
    output_0.coarseVertex_0._S1 = _S3.normal_0;
    output_0._S2 = (((vec4<f32>(_S3.position_0, 1.0f)) * (mat4x4<f32>(Uniforms_0.modelViewProjection_0.data_0[i32(0)][i32(0)], Uniforms_0.modelViewProjection_0.data_0[i32(1)][i32(0)], Uniforms_0.modelViewProjection_0.data_0[i32(2)][i32(0)], Uniforms_0.modelViewProjection_0.data_0[i32(3)][i32(0)], Uniforms_0.modelViewProjection_0.data_0[i32(0)][i32(1)], Uniforms_0.modelViewProjection_0.data_0[i32(1)][i32(1)], Uniforms_0.modelViewProjection_0.data_0[i32(2)][i32(1)], Uniforms_0.modelViewProjection_0.data_0[i32(3)][i32(1)], Uniforms_0.modelViewProjection_0.data_0[i32(0)][i32(2)], Uniforms_0.modelViewProjection_0.data_0[i32(1)][i32(2)], Uniforms_0.modelViewProjection_0.data_0[i32(2)][i32(2)], Uniforms_0.modelViewProjection_0.data_0[i32(3)][i32(2)], Uniforms_0.modelViewProjection_0.data_0[i32(0)][i32(3)], Uniforms_0.modelViewProjection_0.data_0[i32(1)][i32(3)], Uniforms_0.modelViewProjection_0.data_0[i32(2)][i32(3)], Uniforms_0.modelViewProjection_0.data_0[i32(3)][i32(3)]))));
    var _S4 : VertexStageOutput_0;
    _S4.color_0 = output_0.coarseVertex_0._S1;
    _S4.sv_position_0 = output_0._S2;
    return _S4;
}

struct Fragment_0
{
    @location(0) color_1 : vec4<f32>,
};

struct pixelInput_0
{
    @location(0) _S5 : vec3<f32>,
};

struct pixelInput_1
{
     coarseVertex_1 : CoarseVertex_0,
};

@fragment
fn fragmentMain( _S6 : pixelInput_0) -> Fragment_0
{
    var _S7 : pixelInput_1;
    _S7.coarseVertex_1._S1 = _S6._S5;
    var output_1 : Fragment_0;
    output_1.color_1 = vec4<f32>(_S7.coarseVertex_1._S1, 1.0f);
    return output_1;
}

