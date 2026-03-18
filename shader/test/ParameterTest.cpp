#include <gtest/gtest.h>
#include <array>
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
        const SlangCompilerDesc desc{.paths = path};
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
        .semanticNameIdx = ~0u,
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
        .semanticNameIdx = ~0u,
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
        .semanticNameIdx = ~0u,
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
        uint32_t paramIdx = shdr.indices[vertexEntry.ioStartIndex + i];
        const sa::ShaderParameter actual = shdr.parameters[paramIdx];
        const sa::ShaderParameter expected = kExpectedInputParameters[i];
        EXPECT_EQ(actual.location, expected.location);
        EXPECT_EQ(actual.semanticNameIdx, expected.semanticNameIdx);
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
        .semanticNameIdx = ~0u,
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
        .semanticNameIdx = ~0u,
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
        .semanticNameIdx = ~0u,
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
        .semanticNameIdx = 4,
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
        uint32_t paramIdx = shdr.indices[vertexEntry.ioStartIndex + i];
        const sa::ShaderParameter actual = shdr.parameters[paramIdx];
        const sa::ShaderParameter expected = kExpectedStructInputParameters[i];
        EXPECT_EQ(actual.location, expected.location);
        EXPECT_EQ(actual.variable.shape.vector.length, expected.variable.shape.vector.length);
        EXPECT_EQ(actual.variable.scalarType, expected.variable.scalarType);
        EXPECT_EQ(actual.variable.kind, expected.variable.kind);
        ASSERT_LT(actual.variable.nameIdx, shdr.nameTable.size());
        EXPECT_EQ(shdr.nameTable[actual.variable.nameIdx],
                  kStructNameTable[expected.variable.nameIdx]);
        if (actual.semanticNameIdx < shdr.nameTable.size())
        {
            EXPECT_EQ(shdr.nameTable[actual.semanticNameIdx],
                      kStructNameTable[expected.semanticNameIdx]);
        }
    }

    const sa::EntryPoint& fragmnetEntry = shdr.entryPoints[1];
    ASSERT_EQ(shdr.nameTable[fragmnetEntry.nameIdx], "fragmentMain");
    ASSERT_EQ(fragmnetEntry.ioCount, 1);
}