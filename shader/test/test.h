#include <array>

#include <ShaderAssetFormat.h>

constexpr uint32_t kExpectedNoBindBindingNum = 0;
const char* kNoBindCode = R"(

#include <ShaderInterop.h>

import Mesh;


struct CoarseVertex {
    float3 color;
};

// Output of the fragment shader
struct Fragment {
    float4 color;
};

// Vertex  Shader
struct VertexStageOutput {
    CoarseVertex coarseVertex : CoarseVertex;
    float4 sv_position : SV_Position;
};

[shader("vertex")]
VertexStageOutput vertexMain(AssembledVertex assembledVertex) {
    VertexStageOutput output;

    float3 position = assembledVertex.position;
    float3 color = assembledVertex.normal;

    output.coarseVertex.color = color;
    output.sv_position = float4(position, 1.0);

    return output;
}

// Fragment Shader

[shader("fragment")]
Fragment fragmentMain(CoarseVertex coarseVertex: CoarseVertex) : SV_Target {
    float3 color = coarseVertex.color;

    Fragment output;
    output.color = float4(color, 1.0);
    return output;
}
)";

constexpr uint32_t kDebugCodeExpectedBindingNumber = 1;
constexpr uint32_t kDebugCodeExpectedUniformsBufferSize = 80;
const char* kDebugCode = R"(
#include <ShaderInterop.h>

[[vk::binding(0, BindSlot::Global)]]
cbuffer Uniforms {
    float4x4 modelViewProjection;
    float3 cameraPos;
    float time;
}

// Per-vertex attributes to be assembled from bound vertex buffers.
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

// Output of the vertex shader, and input to the fragment shader.
struct CoarseVertex {
    float3 color;
};

// Output of the fragment shader
struct Fragment {
    float4 color;
};

// Vertex  Shader
struct VertexStageOutput {
    CoarseVertex coarseVertex : CoarseVertex;
    float4 sv_position : SV_Position;
};

[shader("vertex")]
VertexStageOutput vertexMain(AssembledVertex assembledVertex) {
    VertexStageOutput output;

    float3 position = assembledVertex.position;
    float3 color = assembledVertex.normal;

    output.coarseVertex.color = color;
    output.sv_position = mul(modelViewProjection, float4(position, 1.0));

    return output;
}

// Fragment Shader

[shader("fragment")]
Fragment fragmentMain(CoarseVertex coarseVertex: CoarseVertex) : SV_Target {
    float3 color = coarseVertex.color;

    Fragment output;
    output.color = float4(color, 1.0);
    return output;
}
)";

const char* kResourcesInStruct = R"(
import Mesh;

struct PBRMaterial {
    Texture2D albedo;    // Binding N
    Texture2D roughness; // Binding N+1
    Texture2D normal;    // Binding N+2
};

[[vk::binding(0, 0)]] 
PBRMaterial myMaterial;

[[shader("vertex")]]
float4 vertexMain(AssembledVertex vertex) {
    return float4(vertex.normal, 1);
}
)";

constexpr uint32_t kParameterBlockExpectedBindingSize = 3;
const std::array<core::ShaderAssetFormat::Binding, kParameterBlockExpectedBindingSize>
    kParameterBlockExpectedResourceTypes{
        core::ShaderAssetFormat::Binding{
            .set = 0,
            .binding = 0,
            .resourceType = core::ShaderAssetFormat::ResourceType::UniformBuffer,
        },
        core::ShaderAssetFormat::Binding{
            .set = 0,
            .binding = 1,
            .resourceType = core::ShaderAssetFormat::ResourceType::Texture,
        },
        core::ShaderAssetFormat::Binding{
            .set = 0,
            .binding = 2,
            .resourceType = core::ShaderAssetFormat::ResourceType::Sampler,
        },
    };
const char* kParameterBlock = R"(
import Mesh;

struct PostProcessSettings {
    Texture2D inputTex; 
    SamplerState linearSampler;
    float3 temp;
};

ParameterBlock<PostProcessSettings> postEffect : register(space3);

[[shader("vertex")]]
float4 vertexMain(AssembledVertex vertex) {
    return float4(vertex.normal, 1);
}
)";

constexpr uint32_t kComplexTestExpectedBindingSize = 7;
const std::array<core::ShaderAssetFormat::Binding, kComplexTestExpectedBindingSize>
    kComplexTestExpectedBindings{
        core::ShaderAssetFormat::Binding{
            // cbuffer PerFrameUniforms
            .set = 0,
            .binding = 0,
            .resourceType = core::ShaderAssetFormat::ResourceType::UniformBuffer,
            .resource = core::ShaderAssetFormat::Resource::Buffer(80),
        },
        core::ShaderAssetFormat::Binding{
            // TextureSet gExtraTextures : register(space0) .albedoMap
            .set = 0,
            .binding = 1,
            .resourceType = core::ShaderAssetFormat::ResourceType::Texture,
        },
        core::ShaderAssetFormat::Binding{
            // TextureSet gExtraTextures : register(space0) .normalMap
            .set = 0,
            .binding = 2,
            .resourceType = core::ShaderAssetFormat::ResourceType::Texture,
        },
        core::ShaderAssetFormat::Binding{
            // ParameterBlock<MaterialData> gMaterial : register(space1) auto generated Uniform
            .set = 1,
            .binding = 0,
            .resourceType = core::ShaderAssetFormat::ResourceType::UniformBuffer,
            .resource = core::ShaderAssetFormat::Resource::Buffer(16),
        },
        core::ShaderAssetFormat::Binding{
            // ParameterBlock<MaterialData> gMaterial : register(space1) .textures.albedoMap
            .set = 1,
            .binding = 1,
            .resourceType = core::ShaderAssetFormat::ResourceType::Texture,
        },
        core::ShaderAssetFormat::Binding{
            // ParameterBlock<MaterialData> gMaterial : register(space1) .textures.normalMap
            .set = 1,
            .binding = 2,
            .resourceType = core::ShaderAssetFormat::ResourceType::Texture,
        },
        core::ShaderAssetFormat::Binding{
            // ParameterBlock<MaterialData> gMaterial : register(space1) .smapler
            .set = 1,
            .binding = 3,
            .resourceType = core::ShaderAssetFormat::ResourceType::Sampler,
        },

    };
const char* kComplexTest = R"(
#include <ShaderInterop.h>
struct AssembledVertex {
    float3 vtx;
};

struct CameraData {
    float4x4 viewProjection;
    float3 eyePosition;
    float time;
};

struct TextureSet {
    Texture2D albedoMap;
    Texture2D normalMap;
};

struct MaterialData {
    TextureSet textures;
    SamplerState sampler;
    float roughness;
};

[[vk::binding(0, BindSlot::Global)]]
cbuffer PerFrameUniforms {
    CameraData gCamera;
};

[[vk::binding(0, BindSlot::Material)]]
ParameterBlock<MaterialData> gMaterial; 

[[vk::binding(1, BindSlot::Global)]]
TextureSet gExtraTextures;


[[shader("vertex")]]
float4 vertexMain(AssembledVertex vertex) : SV_Position {
    float4 pos = mul(gCamera.viewProjection, float4(vertex.vtx, 1.0));
    pos.x += gCamera.time;

    float4 matColor = gMaterial.textures.albedoMap.Load(int3(0,0,0));
    
    float r = gMaterial.roughness;

    float4 extraColor = gExtraTextures.normalMap.Load(int3(0,0,0));

    pos.y += matColor.r + r + extraColor.g;

    return pos;
}
)";

const uint32_t kStandardPBR_VS_ExpectedBindingSize = 4;
const std::array<core::ShaderAssetFormat::Binding, kStandardPBR_VS_ExpectedBindingSize>
    kStandardPBR_VS_ExpectedBindings{
        core::ShaderAssetFormat::Binding{
            .set = 0,
            .binding = 0,
            .resourceType = core::ShaderAssetFormat::ResourceType::UniformBuffer,
            .resource = core::ShaderAssetFormat::Resource::Buffer(64),
        },
        core::ShaderAssetFormat::Binding{
            .set = 1,
            .binding = 0,
            .resourceType = core::ShaderAssetFormat::ResourceType::Texture,
        },
        core::ShaderAssetFormat::Binding{
            .set = 1,
            .binding = 1,
            .resourceType = core::ShaderAssetFormat::ResourceType::Sampler,
        },
        core::ShaderAssetFormat::Binding{
            .set = 2,
            .binding = 0,
            .resourceType = core::ShaderAssetFormat::ResourceType::UniformBuffer,
            .resource = core::ShaderAssetFormat::Resource::Buffer(64),
        },
    };

const char* kStandardPBR_VS_Data = R"(
#include <ShaderInterop.h>

[[vk::binding(0, BindSlot::Global)]]
ConstantBuffer<GlobalUniforms> globalUniforms;

// struct MaterialData {
//    Texture2D<float4> baseColorTexture;
//    SamplerState baseColorSampler;
// };
[[vk::binding(0, BindSlot::Material)]]
Texture2D<float4> baseColorTexture;
//ParameterBlock<MaterialData> materialData;
[[vk::binding(1, BindSlot::Material)]]
SamplerState baseColorSampler;

[[vk::binding(0, BindSlot::Instance)]]
ConstantBuffer<InstanceUniforms> instanceUniforms;

// Per-vertex attributes to be assembled from bound vertex buffers.
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

// Output of the vertex shader, and input to the fragment shader.
struct CoarseVertex {
    float2 uv;
};

// Output of the fragment shader
struct Fragment {
    float4 color;
};

// Vertex  Shader
struct VertexStageOutput {
    CoarseVertex coarseVertex : CoarseVertex;
    float4 sv_position : SV_Position;
};

[shader("vertex")]
VertexStageOutput vertexMain(AssembledVertex assembledVertex) {
    VertexStageOutput output;

    float3 position = assembledVertex.position;
    float3 color = assembledVertex.normal;

    output.coarseVertex.uv = assembledVertex.texcoord;
    output.sv_position = mul(globalUniforms.viewProj, float4(position, 1.0));

    return output;
}

// Fragment Shader
[shader("fragment")]
Fragment fragmentMain(CoarseVertex coarseVertex: CoarseVertex) : SV_Target {
    Fragment output;
    output.color =
        baseColorTexture.Sample(baseColorSampler, coarseVertex.uv);
    return output;
}

)";