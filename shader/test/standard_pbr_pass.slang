#include <ShaderInterop.h>
#include "core_interface.slang"




struct AssembledVertex {
    [[vk::location(ShaderLoc::Position)]]
    float3 position : POSITION;
    [[vk::location(ShaderLoc::Normal)]]
    float3 normal : NORMAL;
    [[vk::location(ShaderLoc::UV)]]
    float2 texcoord : TEXCOORD_0;
    [[vk::location(ShaderLoc::Tangent)]]
    float4 tangent : TANGENT;
};
struct CoarseVertex {
    float2 uv;
};

struct VertexStageOutput {
    CoarseVertex coarseVertex : CoarseVertex;
    float4 sv_position : SV_Position;
};

struct Fragment {
    float4 color;
};

struct AlbedoMaterial : IMaterial {
    typealias DataType = struct {
        Texture2D diffuseMap;
        SamplerState linearSampler;
    };
    static SurfaceAttributes evaluateSurfaceAttributes(DataType data,
                                                       float3 worldPosition,
                                                       float3 worldNormal,
                                                       float2 uv) {
        SurfaceAttributes attributes;
        attributes.baseColor = data.diffuseMap.Sample(data.linearSampler, uv).rgb;
        return attributes;
    }
};

struct ForwardRenderPass<Material : IMaterial> : IRenderPass {
    typealias M = Material;

    typealias VertexInputType = AssembledVertex;
    typealias VertexOutputType = VertexStageOutput;

    typealias FragmentInputType = CoarseVertex;
    typealias FragmentOutputType = Fragment;

    typealias GlobalResourcesType = GlobalResources;
    typealias PassResourceType = EmptyData;

    static VertexOutputType vertex(GlobalResourcesType globalResources,
                                   M.DataType material,
                                   PassResourceType passResources,
                                   VertexInputType input) {
        VertexOutputType output;
        float3 position = input.position;
        float3 color = input.normal;
        output.coarseVertex.uv = input.texcoord;
        output.sv_position = mul(globalResources.viewProjMatrix, float4(position, 1.0));
        return output;
    }

    static FragmentOutputType fragment(GlobalResourcesType globalResources,
                                       M.DataType material,
                                       PassResourceType passResources,
                                       FragmentInputType input) {
        FragmentOutputType output;

        SurfaceAttributes attributes = M.evaluateSurfaceAttributes(material, float3(0, 0, 0), float3(0, 0, 1), input.uv);
        output.color = float4(attributes.baseColor, 1.0f);
        return output;
    }

}

[[vk::binding(0, BindSlot::Instance)]]
ConstantBuffer<InstanceUniforms> instanceUniforms;
