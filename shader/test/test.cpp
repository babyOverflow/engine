#include <gtest/gtest.h>
#include <slang-com-ptr.h>
#include <slang.h>
#include <filesystem>
#include <print>
#include <source_location>
#include "SlangCompiler.h"

#include "ShaderInterop.h"
#include "test.h"

using namespace slangCompiler;

using sa = core::ShaderAsset;

class SlangCompilerTest : public testing::Test {
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

std::expected<SlangCompiler, Error> SlangCompilerTest::compiler = std::unexpected(Error{});

TEST_F(SlangCompilerTest, EmptyCodeFromString) {
    auto result = compiler->CompileFromString("", "vertexMain");
    EXPECT_FALSE(result.has_value());
    auto error = result.error();

    EXPECT_EQ(error.type, slangCompiler::ErrorType::EntryPointNotFound);
    std::print("{}", error.message);
}

TEST_F(SlangCompilerTest, NoBinding) {
    auto result = compiler->CompileFromString(kNoBindCode, "vertexMain");
    ASSERT_TRUE(result.has_value()) << result.error().message;

    CompileResult shrd = result.value();
    EXPECT_EQ(shrd.header.magicNumber, core::ShaderAsset::SHADER_ASSET_MAGIC);
    EXPECT_EQ(shrd.header.bindingCount, kExpectedNoBindBindingNum);
    EXPECT_EQ(shrd.bindings.size(), kExpectedNoBindBindingNum);
}

TEST_F(SlangCompilerTest, DebugCodeCompilation) {
    auto result = compiler->CompileFromString(kDebugCode, "vertexMain");
    ASSERT_TRUE(result.has_value()) << result.error().message;

    CompileResult shrd = result.value();
    EXPECT_EQ(shrd.header.magicNumber, core::ShaderAsset::SHADER_ASSET_MAGIC);
    ASSERT_EQ(shrd.header.bindingCount, kDebugCodeExpectedBindingNumber);
    EXPECT_EQ(shrd.header.bindingCount, shrd.bindings.size());

    auto binding = shrd.bindings[0];

    EXPECT_EQ(binding.binding, 0);
    EXPECT_EQ(binding.resourceType, sa::ResourceType::UniformBuffer);
    EXPECT_EQ(binding.bufferSize, kDebugCodeExpectedUniformsBufferSize);
}

TEST_F(SlangCompilerTest, ThreeTextureInOneStruct) {
    auto result = compiler->CompileFromString(kResourcesInStruct, "vertexMain");
    ASSERT_TRUE(result.has_value()) << result.error().message;

    CompileResult shrd = result.value();
    EXPECT_EQ(shrd.header.bindingCount, 3);
    for (uint32_t i = 0; i < shrd.bindings.size(); ++i) {
        const auto& binding = shrd.bindings[i];
        EXPECT_EQ(binding.binding, i);
        EXPECT_EQ(binding.resourceType, sa::ResourceType::Texture);
    }
}

TEST_F(SlangCompilerTest, ParameterBlock) {
    auto result = compiler->CompileFromString(kParameterBlock, "vertexMain");
    ASSERT_TRUE(result.has_value()) << result.error().message;

    CompileResult shrd = result.value();
    EXPECT_EQ(shrd.header.bindingCount, kParameterBlockExpectedBindingSize);
    for (uint32_t i = 0; i < shrd.bindings.size(); ++i)
    {
        const auto& binding = shrd.bindings[i];
        const auto& expected = kParameterBlockExpectedResourceTypes[i];
        EXPECT_EQ(binding.set, expected.set);
        EXPECT_EQ(binding.binding, expected.binding);
        EXPECT_EQ(binding.resourceType, expected.resourceType);
    }
}

TEST_F(SlangCompilerTest, ComplexTest)
{
    auto result = compiler->CompileFromString(kComplexTest, "vertexMain");
    ASSERT_TRUE(result.has_value()) << result.error().message;

    CompileResult shrd = result.value();
    std::println("{}", shrd.warning);
    EXPECT_EQ(shrd.header.bindingCount, kComplexTestExpectedBindingSize);
    for (uint32_t i = 0; i < shrd.bindings.size(); ++i) {
        const auto& actual = shrd.bindings[i];
        const auto& expected = kComplexTestExpectedBindings[i];
        EXPECT_EQ(actual.set, expected.set)
            << "\033[32m" << actual.name << "\033[0m set number should be " << expected.set;
        EXPECT_EQ(actual.binding, expected.binding)
            << "\033[32m" << actual.name << "\033[0m binding number should be " << expected.binding;
        EXPECT_EQ(actual.bufferSize, expected.bufferSize);
        EXPECT_EQ(actual.resourceType, expected.resourceType);
    }
}

int main(int argc, char** argv) {
    std::cout << ">>> Slang Shader Tests" << "\n";
    ::testing::InitGoogleTest(&argc, argv);

    int result = RUN_ALL_TESTS();

    std::cout << ">>> result: " << result << "\n";
    return result;
}