#include <gtest/gtest.h>
#include <slang-com-ptr.h>
#include <slang.h>
#include <filesystem>
#include <fstream>
#include <print>
#include <source_location>
#include "SlangCompiler.h"

#include "ShaderInterop.h"
#include "test.h"
#include "util.h"

using namespace slangCompiler;

using sa = core::ShaderAssetFormat;
namespace fs = std::filesystem;

class SlangCompilerTest : public testing::Test {
  protected:
    static std::expected<SlangCompiler, Error> compiler;

    static void SetUpTestSuite() {
        // Slang session to include ShaderInterop.h Desc should append current dir to searchPath.
        const auto loc = std::source_location::current();
        const fs::path fullPath(loc.file_name());
        const fs::path dirPath = fullPath.parent_path();

        std::vector<std::filesystem::path> path = {dirPath};
        const SlangCompilerDesc desc{.paths = path};
        compiler = SlangCompiler::Create(desc);
        ASSERT_TRUE(compiler.has_value());
    }
};

class SlangCompilerIOTest : public SlangCompilerTest {
  protected:
    const std::string test_filepath = "test_config_output.txt";

    void SetUp() override {
        // Ensure a clean state before the test starts
        if (fs::exists(test_filepath)) {
            fs::remove(test_filepath);
        }
    }

    // Runs after every TEST_F
    void TearDown() override {
        // Clean up the generated file so we don't pollute the filesystem
        if (fs::exists(test_filepath)) {
            fs::remove(test_filepath);
        }
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
    EXPECT_EQ(shrd.bindings.size(), kExpectedNoBindBindingNum);
}

TEST_F(SlangCompilerTest, DebugCodeCompilationWithEntry) {
    auto result = compiler->CompileFromString(kDebugCode, "vertexMain");
    ASSERT_TRUE(result.has_value()) << result.error().message;

    CompileResult shrd = result.value();
    EXPECT_EQ(shrd.entryPoints.size(), 1);
    ASSERT_EQ(shrd.bindings.size(), kDebugCodeExpectedBindingNumber);

    auto binding = shrd.bindings[0];

    EXPECT_EQ(binding.binding, 0);
    EXPECT_EQ(binding.resourceType, sa::ResourceType::UniformBuffer);
    EXPECT_EQ(binding.resource.buffer.bufferSize, kDebugCodeExpectedUniformsBufferSize);
}

TEST_F(SlangCompilerTest, DebugCodeCompilation) {
    auto result = compiler->CompileFromString(kDebugCode);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    const CompileResult shrd = result.value();
    ASSERT_EQ(shrd.entryPoints.size(), 2);
    ASSERT_EQ(shrd.nameTable[shrd.entryPoints[0].nameIdx], "vertexMain");
    ASSERT_EQ(shrd.nameTable[shrd.entryPoints[1].nameIdx], "fragmentMain");

    const sa::EntryPoint& ve = shrd.entryPoints[0];
    ASSERT_EQ(ve.bindingCount, 1);

    const sa::Binding& binding = shrd.bindings[shrd.indices[ve.bindingStartIndex]];
    EXPECT_EQ(binding.binding, 0);
    EXPECT_EQ(binding.resourceType, sa::ResourceType::UniformBuffer);
    EXPECT_EQ(binding.resource.buffer.bufferSize, kDebugCodeExpectedUniformsBufferSize);
}

TEST_F(SlangCompilerTest, ThreeTextureInOneStruct) {
    auto result = compiler->CompileFromString(kResourcesInStruct, "vertexMain");
    ASSERT_TRUE(result.has_value()) << result.error().message;

    CompileResult shrd = result.value();
    EXPECT_EQ(shrd.bindings.size(), 3);
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
    EXPECT_EQ(shrd.bindings.size(), kParameterBlockExpectedBindingSize);
    for (uint32_t i = 0; i < shrd.bindings.size(); ++i) {
        const auto& binding = shrd.bindings[i];
        const auto& expected = kParameterBlockExpectedResourceTypes[i];
        EXPECT_EQ(binding.set, expected.set);
        EXPECT_EQ(binding.binding, expected.binding);
        EXPECT_EQ(binding.resourceType, expected.resourceType);
    }
}

TEST_F(SlangCompilerTest, ComplexTest) {
    auto result = compiler->CompileFromString(kComplexTest, "vertexMain");
    ASSERT_TRUE(result.has_value()) << result.error().message;

    CompileResult shrd = result.value();
    std::println("{}", shrd.warning);
    EXPECT_EQ(shrd.bindings.size(), kComplexTestExpectedBindingSize);
    for (uint32_t i = 0; i < shrd.bindings.size(); ++i) {
        const auto& actual = shrd.bindings[i];
        const auto& expected = kComplexTestExpectedBindings[i];
        EXPECT_EQ(actual.set, expected.set) << "\033[32m" << shrd.nameTable[actual.nameIdx]
                                            << "\033[0m set number should be " << expected.set;
        EXPECT_EQ(actual.binding, expected.binding)
            << "\033[32m" << shrd.nameTable[actual.nameIdx] << "\033[0m binding number should be "
            << expected.binding;
        ASSERT_EQ(actual.resourceType, expected.resourceType);
        if (actual.resourceType == sa::ResourceType::UniformBuffer) {
            EXPECT_EQ(actual.resource.buffer.bufferSize, expected.resource.buffer.bufferSize);
        }
    }
}

TEST_F(SlangCompilerTest, StandardPBRWithEntry) {
    auto result = compiler->CompileFromString(kStandardPBR_Data, "vertexMain");
    ASSERT_TRUE(result.has_value()) << result.error().message;

    CompileResult shrd = result.value();
    std::println("{}", shrd.warning);
    EXPECT_EQ(shrd.bindings.size(), kStandardPBR_ExpectedBindingSize);
    for (uint32_t i = 0; i < shrd.bindings.size(); ++i) {
        const auto& actual = shrd.bindings[i];
        const auto& expected = kStandardPBR_ExpectedBindings[i];

        EXPECT_EQ(actual.set, expected.set) << "\033[32m" << shrd.nameTable[actual.nameIdx]
                                            << "\033[0m set number should be " << expected.set;
        EXPECT_EQ(actual.binding, expected.binding)
            << "\033[32m" << shrd.nameTable[actual.nameIdx] << "\033[0m binding number should be "
            << expected.binding;
        ASSERT_EQ(actual.resourceType, expected.resourceType);
        if (actual.resourceType == sa::ResourceType::UniformBuffer) {
            EXPECT_EQ(actual.resource.buffer.bufferSize, expected.resource.buffer.bufferSize);
        }
    }
}

auto MakeNameFinder(std::vector<std::string>& nameTable) {
    return
        [&nameTable](const auto& entity) -> std::string_view { return nameTable[entity.nameIdx]; };
}

TEST_F(SlangCompilerIOTest, StandardPBR) {
    auto result = compiler->CompileFromString(kStandardPBR_Data);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    WriteAssetToFile(test_filepath, result.value());
    // TODO!(Sunghyun;2026-03-18; move test to root and verify load function)
}

TEST_F(SlangCompilerTest, StandardPBR) {
    auto result = compiler->CompileFromString(kStandardPBR_Data);
    ASSERT_TRUE(result.has_value()) << result.error().message;

    CompileResult shdr = result.value();
    std::println("{}", shdr.warning);
    ASSERT_EQ(shdr.entryPoints.size(), 2);
    auto getShdrNames = MakeNameFinder(shdr.nameTable);
    ASSERT_EQ(getShdrNames(shdr.entryPoints[0]), "vertexMain");
    ASSERT_EQ(getShdrNames(shdr.entryPoints[1]), "fragmentMain");

    const sa::EntryPoint& ve = shdr.entryPoints[0];
    {
        ASSERT_EQ(ve.bindingCount, kStandardPBR_ExpectedBindingsSize_VS);
        for (uint32_t i = 0; i < ve.bindingCount; ++i) {
            const auto& actual = shdr.bindings[shdr.indices[ve.bindingStartIndex + i]];
            const auto& expected = kStandardPBR_ExpectedBindings_VS[i];

            EXPECT_EQ(actual.set, expected.set) << "\033[32m" << shdr.nameTable[actual.nameIdx]
                                                << "\033[0m set number should be " << expected.set;
            EXPECT_EQ(actual.binding, expected.binding)
                << "\033[32m" << getShdrNames(actual) << "\033[0m binding number should be "
                << expected.binding;
            ASSERT_EQ(actual.resourceType, expected.resourceType);
            if (actual.resourceType == sa::ResourceType::UniformBuffer) {
                EXPECT_EQ(actual.resource.buffer.bufferSize, expected.resource.buffer.bufferSize);
            }
        }
    }
    const sa::EntryPoint& fe = shdr.entryPoints[1];
    {
        ASSERT_EQ(fe.bindingCount, 2);
        for (uint32_t i = 0; i < fe.bindingCount; ++i) {
            const auto& actual = shdr.bindings[shdr.indices[fe.bindingStartIndex + i]];
            const auto& expected = kStandardPBR_ExpectedBindings_FS[i];

            EXPECT_EQ(actual.set, expected.set) << "\033[32m" << shdr.nameTable[actual.nameIdx]
                                                << "\033[0m set number should be " << expected.set;
            EXPECT_EQ(actual.binding, expected.binding)
                << "\033[32m" << getShdrNames(actual) << "\033[0m binding number should be "
                << expected.binding;
            ASSERT_EQ(actual.resourceType, expected.resourceType);
            if (actual.resourceType == sa::ResourceType::UniformBuffer) {
                EXPECT_EQ(actual.resource.buffer.bufferSize, expected.resource.buffer.bufferSize);
            }
        }
    }
}

int main(int argc, char** argv) {
    std::cout << ">>> Slang Shader Tests" << "\n";
    ::testing::InitGoogleTest(&argc, argv);

    int result = RUN_ALL_TESTS();

    std::cout << ">>> result: " << result << "\n";
    return result;
}