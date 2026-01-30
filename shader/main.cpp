#include <filesystem>
#include <fstream>
#include <print>
#include <string>
#include <ranges>
#include "SlangCompiler.h"

namespace fs = std::filesystem;
using sa = core::ShaderAssetFormat;
using namespace slang;
using namespace slangCompiler;

void PrintUsage() {
    std::println(
        "Usage: shader_baker -i <input_file> -e <entry_point> -o <output_file> [-I <include_path>...]");
}

bool WriteAssetToFile(const fs::path& outputPath, const CompileResult& result) {
    std::ofstream file(outputPath, std::ios::binary);
    if (!file.is_open()) {
        std::println(stderr, "Error: Failed to open output file: {}", outputPath.string());
        return false;
    }

    file.write(reinterpret_cast<const char*>(&result.header), sizeof(sa::Header));

    if (!result.bindings.empty())
    {
        file.write(reinterpret_cast<const char*>(result.bindings.data()),
                   sizeof(sa::Binding) * result.bindings.size());
    }

    if (!result.sourceBlob.empty())
    {
        file.write(reinterpret_cast<const char*>(result.sourceBlob.data()),
                   result.sourceBlob.size());
    }

    file.close();
    return true;
}

int main(int argc, char** argv) {
    if (argc < 5) {
        PrintUsage();
        return 1;
    }
    fs::path inputPath;
    fs::path outputPath;
    std::string_view entryPoint;
    std::vector<fs::path> includePaths;

    for (int i = 0; i < argc; ++i) {
        std::string_view arg = argv[i];
        if (arg == "-i" && i + 1 < argc) {
            inputPath = argv[++i];
        } else if (arg == "-o" && i + 1 < argc) {
            outputPath = argv[++i];
        } else if (arg == "-e" && i + 1 < argc) {
            entryPoint = argv[++i];
        } else if (arg == "-I" && i + 1 < argc) {
            includePaths.push_back(argv[++i]);
        }
    }

    if (inputPath.empty() || entryPoint.empty() || outputPath.empty()) {
        std::println(stderr, "Error: Missing required arguent");
        PrintUsage();
        return 1;
    }


    SlangCompilerDesc desc{.paths = includePaths};
    auto compilerOrError = SlangCompiler::Create(desc);
    if (!compilerOrError.has_value()) {
        std::println(stderr, "Error! Failed to initialize compiler: {}",
                     compilerOrError.error().message);
        return 1;
    }
    SlangCompiler compiler = compilerOrError.value();

    std::println("Compiling {} ({}) ...", inputPath.string(), entryPoint);

    auto resultOrError = compiler.Compile(inputPath.string(), std::string(entryPoint));
    if (!resultOrError.has_value()) {
        std::println(stderr, "Error! Compilation failed {}({}): {}", inputPath.string(), entryPoint,
                     resultOrError.error().message);
        return 1;
    }

    const CompileResult compileResult = resultOrError.value();

    if (!compileResult.warning.empty()) {
        std::println("Warning!\n {}", compileResult.warning);
    }

    if (WriteAssetToFile(outputPath, compileResult)) {
        std::println("Succsfully baked: {}", outputPath.string());
        std::println(" - Bindings: {}", compileResult.header.bindingCount);
        std::println(" - Code size: {}", compileResult.header.shaderSize);
        return 0;
    } else {
        return 1;
    }
}