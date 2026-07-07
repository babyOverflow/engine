#include <gtest/gtest.h>
#include <array>
#include <magic_enum/magic_enum.hpp>
#include <source_location>

#include "SlangCompiler.h"

using namespace slangCompiler;

using sa = core::ShaderAssetFormat;

class ParameterTest : public testing::Test {
  protected:
    static std::expected<SlangCompiler, Error> compiler;

    static void SetUpTestSuite() {
        // Slang session to include ShaderInterop.h Desc should append current dir to searchPath.
        const auto loc = std::source_location::current();
        const std::filesystem::path fullPath(loc.file_name());
        const std::filesystem::path dirPath = fullPath.parent_path();

        std::vector<std::filesystem::path> path = {dirPath};
        const SlangCompilerDesc desc{.paths = path,
                                     .entryTemplatePaths = {dirPath / "entry.slang"}};
        compiler = SlangCompiler::Create(desc);
        ASSERT_TRUE(compiler.has_value());
    }
};

std::expected<SlangCompiler, Error> ParameterTest::compiler = std::unexpected(Error{});

const char* kParameter = R"(


struct Material
{
    int albedoTextureIndex;
    int normalTextureIndex;
}

struct Scene
{
    StructuredBuffer<Material> materials;
    Texture2D textures[128];
}

struct Camera
{
    float4x4 mvp;
}

ParameterBlock<Scene> scene;
ConstantBuffer<Camera> camera;
SamplerState samplerState;

struct VOut
{
    float4 position : SV_Position;
    float3 normal;
    float2 uv;
}

[shader("vertex")]
VOut vertexMain(
    float3 position,
    float3 normal,
    float2 uv
)
{
    VOut output;
    output.position = mul(camera.mvp, float4(position, 1.0));
    output.normal = normal;
    output.uv = uv;
    return output;
}

[shader("fragment")]
float4 fragmentMain(VOut input) : SV_Target
{
    let texture = scene.textures[scene.materials[0].albedoTextureIndex];
    let color = texture.Sample(samplerState, input.uv);
    return color;
}

)";

const uint32_t kParameterCount = 3;
const std::array<std::string, kParameterCount> kNameTable = {"position", "normal", "uv"};
const std::array<sa::ShaderParameter, kParameterCount> kExpectedInputParameters = {
    sa::ShaderParameter{
        .variable =
            sa::Variable{
                .kind = sa::Kind::Vector,
                .scalarType = sa::ScalarType::F32,
                .shape = sa::Shape::Vector(3),
                .nameIdx = 0,
            },
        .location = 0,
    },
    sa::ShaderParameter{
        .variable =
            sa::Variable{
                .kind = sa::Kind::Vector,
                .scalarType = sa::ScalarType::F32,
                .shape = sa::Shape::Vector(3),
                .nameIdx = 1,
            },
        .location = 1,
    },
    sa::ShaderParameter{
        .variable =
            sa::Variable{
                .kind = sa::Kind::Vector,
                .scalarType = sa::ScalarType::F32,
                .shape = sa::Shape::Vector(2),
                .nameIdx = 2,
            },
        .location = 2,
    },
};

TEST_F(ParameterTest, ExtractsVertexInputsFromStruct) {
    const auto result = compiler->CompileFromString(kParameter);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    CompileResult shdr = result.value();

    ASSERT_EQ(shdr.entryPoints.size(), 2);
    const sa::EntryPoint& vertexEntry = shdr.entryPoints[0];
    ASSERT_EQ(shdr.nameTable[vertexEntry.nameIdx], "vertexMain");
    ASSERT_EQ(vertexEntry.ioCount, kParameterCount);
    for (uint32_t i = 0; i < kParameterCount; ++i) {
        const sa::ShaderParameter actual = shdr.parameters[i];
        const sa::ShaderParameter expected = kExpectedInputParameters[i];
        EXPECT_EQ(actual.location, expected.location);
        EXPECT_EQ(actual.variable.shape.vector.length, expected.variable.shape.vector.length);
        EXPECT_EQ(actual.variable.scalarType, expected.variable.scalarType);
        EXPECT_EQ(actual.variable.kind, expected.variable.kind);
        ASSERT_LT(actual.variable.nameIdx, shdr.nameTable.size());
        EXPECT_EQ(shdr.nameTable[actual.variable.nameIdx], kNameTable[expected.variable.nameIdx]);
    }

    const sa::EntryPoint& fragmnetEntry = shdr.entryPoints[1];
    ASSERT_EQ(shdr.nameTable[fragmnetEntry.nameIdx], "fragmentMain");
    ASSERT_EQ(fragmnetEntry.ioCount, 1);
}

const char* kStructParameter = R"(
struct Vertex {
    float3 position;
    float3 normal;
    float2 uv;
}

struct Material {
    int albedoTextureIndex;
    int normalTextureIndex;
}

struct Scene {
    StructuredBuffer<Material> materials;
    Texture2D textures[128];
}

struct Camera {
    float4x4 mvp;
}

ParameterBlock<Scene>
    scene;
ConstantBuffer<Camera> camera;
SamplerState samplerState;

struct VOut {
    float4 position : SV_Position;
    float3 normal;
    float2 uv;
}

    [shader("vertex")] VOut
    vertexMain(Vertex input, uint32_t ddd: SV_InstanceID) {
    VOut output;
    output.position = mul(camera.mvp, float4(input.position, 1.0));
    output.normal = input.normal;
    output.uv = input.uv;
    return output;
}

[shader("fragment")] float4 fragmentMain(VOut input) : SV_Target {
    let texture = scene.textures[scene.materials[0].albedoTextureIndex];
    let color = texture.Sample(samplerState, input.uv);
    return color;
}
)";

const uint32_t kStructParameterCount = 4;
const std::array<std::string, 5> kStructNameTable = {"input.position", "input.normal", "input.uv",
                                                     "ddd", "SV_INSTANCEID"};
const std::array<sa::ShaderParameter, kStructParameterCount> kExpectedStructInputParameters = {
    sa::ShaderParameter{
        .variable =
            sa::Variable{
                .kind = sa::Kind::Vector,
                .scalarType = sa::ScalarType::F32,
                .shape = sa::Shape::Vector(3),
                .nameIdx = 0,
            },
        .location = 0,
    },
    sa::ShaderParameter{
        .variable =
            sa::Variable{
                .kind = sa::Kind::Vector,
                .scalarType = sa::ScalarType::F32,
                .shape = sa::Shape::Vector(3),
                .nameIdx = 1,
            },
        .location = 1,
    },
    sa::ShaderParameter{
        .variable =
            sa::Variable{
                .kind = sa::Kind::Vector,
                .scalarType = sa::ScalarType::F32,
                .shape = sa::Shape::Vector(2),
                .nameIdx = 2,
            },
        .location = 2,
    },
    sa::ShaderParameter{
        .variable =
            sa::Variable{
                .kind = sa::Kind::Scalar,
                .scalarType = sa::ScalarType::U32,
                .shape = sa::Shape::Scalar(),
                .nameIdx = 3,
            },
        .location = 0,
    },
};

TEST_F(ParameterTest, ExtractsStructInputsFromStruct) {
    const auto result = compiler->CompileFromString(kStructParameter);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    CompileResult shdr = result.value();

    ASSERT_EQ(shdr.entryPoints.size(), 2);
    const sa::EntryPoint& vertexEntry = shdr.entryPoints[0];
    ASSERT_EQ(shdr.nameTable[vertexEntry.nameIdx], "vertexMain");
    ASSERT_EQ(vertexEntry.ioCount, kStructParameterCount);
    for (uint32_t i = 0; i < kStructParameterCount; ++i) {
        const sa::ShaderParameter actual = shdr.parameters[i];
        const sa::ShaderParameter expected = kExpectedStructInputParameters[i];
        EXPECT_EQ(actual.location, expected.location);
        EXPECT_EQ(actual.variable.shape.vector.length, expected.variable.shape.vector.length);
        EXPECT_EQ(actual.variable.scalarType, expected.variable.scalarType);
        EXPECT_EQ(actual.variable.kind, expected.variable.kind);
        ASSERT_LT(actual.variable.nameIdx, shdr.nameTable.size());
        EXPECT_EQ(shdr.nameTable[actual.variable.nameIdx],
                  kStructNameTable[expected.variable.nameIdx]);
    }

    const sa::EntryPoint& fragmnetEntry = shdr.entryPoints[1];
    ASSERT_EQ(shdr.nameTable[fragmnetEntry.nameIdx], "fragmentMain");
    ASSERT_EQ(fragmnetEntry.ioCount, 1);
}

const static std::array<std::string, 6> kPassNameTable = {
    "input.position", "input.normal", "input.uv", "albedo", "normal", "linearDepth"};

constexpr uint32_t kPassParameterCount = 3;
const static std::array<sa::ShaderParameter, kPassParameterCount> kExpectedPassParameters = {
    sa::ShaderParameter{
        .variable =
            sa::Variable{
                .kind = sa::Kind::Vector,
                .scalarType = sa::ScalarType::F32,
                .shape = sa::Shape::Vector(3),
                .nameIdx = 0,
            },
        .location = 0,
        .semantic = core::Semantic::Position,
    },
    sa::ShaderParameter{
        .variable =
            sa::Variable{
                .kind = sa::Kind::Vector,
                .scalarType = sa::ScalarType::F32,
                .shape = sa::Shape::Vector(3),
                .nameIdx = 1,
            },
        .location = 1,
        .semantic = core::Semantic::Normal,
    },
    sa::ShaderParameter{
        .variable =
            sa::Variable{
                .kind = sa::Kind::Vector,
                .scalarType = sa::ScalarType::F32,
                .shape = sa::Shape::Vector(2),
                .nameIdx = 2,
            },
        .location = 2,
        .semantic = core::Semantic::TexCoord0,
    },
};
constexpr uint32_t kPassResultParameterCount = 3;
const static std::array<sa::ShaderParameter, kPassResultParameterCount>
    kExpectedPassResultParameters = {
        sa::ShaderParameter{
            .variable =
                sa::Variable{
                    .kind = sa::Kind::Vector,
                    .scalarType = sa::ScalarType::F32,
                    .shape = sa::Shape::Vector(4),
                    .nameIdx = 3,
                },
            .location = 0,
            .semantic = core::Semantic::Target0,
        },
        sa::ShaderParameter{
            .variable =
                sa::Variable{
                    .kind = sa::Kind::Vector,
                    .scalarType = sa::ScalarType::F32,
                    .shape = sa::Shape::Vector(4),
                    .nameIdx = 4,
                },
            .location = 1,
            .semantic = core::Semantic::Target1,

        },
        sa::ShaderParameter{
            .variable =
                sa::Variable{
                    .kind = sa::Kind::Vector,
                    .scalarType = sa::ScalarType::F32,
                    .shape = sa::Shape::Vector(4),
                    .nameIdx = 5,
                },
            .location = 2,
            .semantic = core::Semantic::Target2,
        },
};

TEST_F(ParameterTest, ExtractsParameterFromIRenderPass) {
    const auto loc = std::source_location::current();
    const std::filesystem::path fullPath(loc.file_name());
    const std::filesystem::path dirPath = fullPath.parent_path();
    std::string targetPath = dirPath.string() + "/dummy.slang";
    const auto result = compiler->CompilePass(targetPath);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    slangCompiler::CompileResult shdr = result.value();

    EXPECT_EQ(shdr.nameTable[shdr.passNameIdx], "GBufferPass");
    EXPECT_EQ(shdr.nameTable[shdr.materialNameIdx], "DifuseMaterial");

    ASSERT_EQ(shdr.entryPoints.size(), 2);
    const sa::EntryPoint& vertexEntry = shdr.entryPoints[0];
    ASSERT_EQ(shdr.nameTable[vertexEntry.nameIdx], "vertexMain");
    const sa::EntryPoint& fragmentEntry = shdr.entryPoints[1];
    ASSERT_EQ(shdr.nameTable[fragmentEntry.nameIdx], "fragmentMain");

    ASSERT_EQ(vertexEntry.ioCount, kPassParameterCount);
    for (uint32_t i = 0; i < kPassParameterCount; ++i) {
        const sa::ShaderParameter actual = shdr.parameters[i];
        const sa::ShaderParameter expected = kExpectedPassParameters[i];
        EXPECT_EQ(actual.location, expected.location);
        EXPECT_EQ(actual.variable.shape.vector.length, expected.variable.shape.vector.length);
        EXPECT_EQ(actual.variable.scalarType, expected.variable.scalarType);
        EXPECT_EQ(actual.variable.kind, expected.variable.kind);
        ASSERT_LT(actual.variable.nameIdx, shdr.nameTable.size());
        EXPECT_EQ(shdr.nameTable[actual.variable.nameIdx],
                  kPassNameTable[expected.variable.nameIdx]);
        EXPECT_EQ(actual.semantic, expected.semantic);
    }

    ASSERT_EQ(fragmentEntry.ioCount, kPassResultParameterCount);
    for (uint32_t i = 0; i < kPassResultParameterCount; ++i) {
        const sa::ShaderParameter actual = shdr.parameters[kPassParameterCount + i];
        const sa::ShaderParameter expected = kExpectedPassResultParameters[i];
        EXPECT_EQ(actual.location, expected.location);
        EXPECT_EQ(actual.variable.shape.vector.length, expected.variable.shape.vector.length);
        EXPECT_EQ(actual.variable.scalarType, expected.variable.scalarType);
        EXPECT_EQ(actual.variable.kind, expected.variable.kind);
        ASSERT_LT(actual.variable.nameIdx, shdr.nameTable.size());
        EXPECT_EQ(shdr.nameTable[actual.variable.nameIdx],
                  kPassNameTable[expected.variable.nameIdx]);
        EXPECT_EQ(actual.semantic, expected.semantic)
            << "Expected semantic: " << magic_enum::enum_name(expected.semantic)
            << ", but got: " << magic_enum::enum_name(actual.semantic) << "\n";
    }
}

const static std::array<std::string, 5> kForwardPassNameTable = {
    "input.position", "input.normal", "input.texcoord", "input.tangent", "color"};

constexpr uint32_t kForwardPassParameterCount = 4;
const static std::array<sa::ShaderParameter, kForwardPassParameterCount>
    kExpectedForwardPassParameters = {
        sa::ShaderParameter{
            .variable =
                sa::Variable{
                    .kind = sa::Kind::Vector,
                    .scalarType = sa::ScalarType::F32,
                    .shape = sa::Shape::Vector(3),
                    .nameIdx = 0,
                },
            .location = 0,
            .semantic = core::Semantic::Position,
        },
        sa::ShaderParameter{
            .variable =
                sa::Variable{
                    .kind = sa::Kind::Vector,
                    .scalarType = sa::ScalarType::F32,
                    .shape = sa::Shape::Vector(3),
                    .nameIdx = 1,
                },
            .location = 1,
            .semantic = core::Semantic::Normal,
        },
        sa::ShaderParameter{
            .variable =
                sa::Variable{
                    .kind = sa::Kind::Vector,
                    .scalarType = sa::ScalarType::F32,
                    .shape = sa::Shape::Vector(2),
                    .nameIdx = 2,
                },
            .location = 2,
            .semantic = core::Semantic::TexCoord0,
        },
        sa::ShaderParameter{
            .variable =
                sa::Variable{
                    .kind = sa::Kind::Vector,
                    .scalarType = sa::ScalarType::F32,
                    .shape = sa::Shape::Vector(4),
                    .nameIdx = 3,
                },
            .location = 3,
            .semantic = core::Semantic::Tangent,
        },
};

constexpr uint32_t kForwardPassResultParameterCount = 1;
const static std::array<sa::ShaderParameter, kForwardPassResultParameterCount>
    kExpectedForwardPassResultParameters = {
        sa::ShaderParameter{
            .variable =
                sa::Variable{
                    .kind = sa::Kind::Vector,
                    .scalarType = sa::ScalarType::F32,
                    .shape = sa::Shape::Vector(4),
                    .nameIdx = 4,
                },
            .location = 0,
            .semantic = core::Semantic::Undefined,
        },
};

TEST_F(ParameterTest, ExtractsParameterFromForwardPass) {
    const auto loc = std::source_location::current();
    const std::filesystem::path fullPath(loc.file_name());
    const std::filesystem::path dirPath = fullPath.parent_path();
    std::string targetPath = dirPath.string() + "/forward_pass.slang";
    const auto result = compiler->CompilePass(targetPath);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    slangCompiler::CompileResult shdr = result.value();

    EXPECT_EQ(shdr.nameTable[shdr.passNameIdx], "ForwardRenderPass");
    EXPECT_EQ(shdr.nameTable[shdr.materialNameIdx], "ForwardMaterial");

    ASSERT_EQ(shdr.entryPoints.size(), 2);
    const sa::EntryPoint& vertexEntry = shdr.entryPoints[0];
    ASSERT_EQ(shdr.nameTable[vertexEntry.nameIdx], "vertexMain");
    const sa::EntryPoint& fragmentEntry = shdr.entryPoints[1];
    ASSERT_EQ(shdr.nameTable[fragmentEntry.nameIdx], "fragmentMain");

    ASSERT_EQ(vertexEntry.ioCount, kForwardPassParameterCount);
    for (uint32_t i = 0; i < kForwardPassParameterCount; ++i) {
        const sa::ShaderParameter actual = shdr.parameters[i];
        const sa::ShaderParameter expected = kExpectedForwardPassParameters[i];
        EXPECT_EQ(actual.location, expected.location);
        EXPECT_EQ(actual.variable.shape.vector.length, expected.variable.shape.vector.length);
        EXPECT_EQ(actual.variable.scalarType, expected.variable.scalarType);
        EXPECT_EQ(actual.variable.kind, expected.variable.kind);
        ASSERT_LT(actual.variable.nameIdx, shdr.nameTable.size());
        EXPECT_EQ(shdr.nameTable[actual.variable.nameIdx],
                  kForwardPassNameTable[expected.variable.nameIdx]);
        EXPECT_EQ(actual.semantic, expected.semantic);
    }

    ASSERT_EQ(fragmentEntry.ioCount, kForwardPassResultParameterCount);
    for (uint32_t i = 0; i < kForwardPassResultParameterCount; ++i) {
        const sa::ShaderParameter actual = shdr.parameters[kForwardPassParameterCount + i];
        const sa::ShaderParameter expected = kExpectedForwardPassResultParameters[i];
        EXPECT_EQ(actual.location, expected.location);
        EXPECT_EQ(actual.variable.shape.vector.length, expected.variable.shape.vector.length);
        EXPECT_EQ(actual.variable.scalarType, expected.variable.scalarType);
        EXPECT_EQ(actual.variable.kind, expected.variable.kind);
        ASSERT_LT(actual.variable.nameIdx, shdr.nameTable.size());
        EXPECT_EQ(shdr.nameTable[actual.variable.nameIdx],
                  kForwardPassNameTable[expected.variable.nameIdx]);
        EXPECT_EQ(actual.semantic, expected.semantic);
    }
}

TEST_F(ParameterTest, ExtractsBindingsFromForwardPass) {
    const auto loc = std::source_location::current();
    const std::filesystem::path fullPath(loc.file_name());
    const std::filesystem::path dirPath = fullPath.parent_path();
    std::string targetPath = dirPath.string() + "/forward_pass.slang";
    const auto result = compiler->CompilePass(targetPath);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    slangCompiler::CompileResult shdr = result.value();

    constexpr uint32_t kExpectedBindingsSize = 6;
    const static std::array<std::string, kExpectedBindingsSize> kForwardPassBindingNameTable = {
        "globalResources",           "material.linearSampler",
        "material.baseColorTexture", "material.metallicRoughnessTexture",
        "material.normalTexture",    "material.depth",
    };

    const static std::array<uint32_t, kExpectedBindingsSize> kForwardPassBindingIDs = {
        core::ToPropertyID("globalResources"),  core::ToPropertyID("linearSampler"),
        core::ToPropertyID("baseColorTexture"), core::ToPropertyID("metallicRoughnessTexture"),
        core::ToPropertyID("normalTexture"),    core::ToPropertyID("depth"),
    };

    const std::array<core::ShaderAssetFormat::Binding, kExpectedBindingsSize> kExpectedBindings{
        core::ShaderAssetFormat::Binding{
            .set = 0,
            .binding = 0,
            .resource = core::ShaderAssetFormat::Resource::Buffer(80),
            .resourceType = core::ShaderAssetFormat::ResourceType::UniformBuffer,
        },
        core::ShaderAssetFormat::Binding{
            .set = 1,
            .binding = 0,
            .resourceType = core::ShaderAssetFormat::ResourceType::Sampler,
        },
        core::ShaderAssetFormat::Binding{
            .set = 1,
            .binding = 1,
            .resource =
                {
                    .texture =
                        core::ShaderAssetFormat::Texture{
                            .type = core::ShaderAssetFormat::TextureType::Float,
                            .viewDimension = core::ShaderAssetFormat::ViewDimension::e2D,
                            .multiSampled = 0},
                },
            .resourceType = core::ShaderAssetFormat::ResourceType::Texture,
        },
        core::ShaderAssetFormat::Binding{
            .set = 1,
            .binding = 2,
            .resource =
                {
                    .texture =
                        core::ShaderAssetFormat::Texture{
                            .type = core::ShaderAssetFormat::TextureType::Float,
                            .viewDimension = core::ShaderAssetFormat::ViewDimension::e2D,
                            .multiSampled = 0},
                },
            .resourceType = core::ShaderAssetFormat::ResourceType::Texture,
        },
        core::ShaderAssetFormat::Binding{
            .set = 1,
            .binding = 3,
            .resource =
                {
                    .texture =
                        core::ShaderAssetFormat::Texture{
                            .type = core::ShaderAssetFormat::TextureType::Float,
                            .viewDimension = core::ShaderAssetFormat::ViewDimension::e2D,
                            .multiSampled = 0},
                },
            .resourceType = core::ShaderAssetFormat::ResourceType::Texture,
        },
        core::ShaderAssetFormat::Binding{
            .set = 1,
            .binding = 4,
            .resource =
                {
                    .texture =
                        core::ShaderAssetFormat::Texture{
                            .type = core::ShaderAssetFormat::TextureType::Depth,
                            .viewDimension = core::ShaderAssetFormat::ViewDimension::e2D,
                            .multiSampled = 0},
                },
            .resourceType = core::ShaderAssetFormat::ResourceType::Texture,
        }};

    ASSERT_EQ(shdr.bindings.size(), kExpectedBindingsSize);
    for (uint32_t i = 0; i < kExpectedBindingsSize; ++i) {
        const auto& actual = shdr.bindings[i];
        const auto& expected = kExpectedBindings[i];

        ASSERT_LT(actual.nameIdx, shdr.nameTable.size());
        EXPECT_EQ(shdr.nameTable[actual.nameIdx], kForwardPassBindingNameTable[i])
            << "Binding name mismatch at index " << i;

        EXPECT_EQ(actual.id, kForwardPassBindingIDs[i]) << "Binding ID mismatch at index " << i;

        EXPECT_EQ(actual.set, expected.set) << "Set mismatch at binding index " << i;
        EXPECT_EQ(actual.binding, expected.binding)
            << "Binding index mismatch at binding index " << i;
        EXPECT_EQ(actual.resourceType, expected.resourceType)
            << "Resource type mismatch at binding index " << i;
        if (actual.resourceType == sa::ResourceType::UniformBuffer) {
            EXPECT_EQ(actual.resource.buffer.bufferSize, expected.resource.buffer.bufferSize)
                << "Buffer size mismatch at binding index " << i;
        } else if (actual.resourceType == sa::ResourceType::Texture) {
            EXPECT_EQ(actual.resource.texture.type, expected.resource.texture.type)
                << "Texture type mismatch at binding index " << i;
            EXPECT_EQ(actual.resource.texture.multiSampled, expected.resource.texture.multiSampled)
                << "Texture multiSampled mismatch at binding index " << i;
            ;
            EXPECT_EQ(actual.resource.texture.viewDimension,
                      expected.resource.texture.viewDimension)
                << "Texture viewDimention mismatch at binding index " << i;
            ;
        }
    }
}
