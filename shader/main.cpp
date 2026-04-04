#include <filesystem>
#include <fstream>
#include <print>
#include <ranges>
#include <string>
#include "SlangCompiler.h"
#include "util.h"

namespace fs = std::filesystem;
using sa = core::ShaderAssetFormat;
using namespace slang;
using namespace slangCompiler;

void PrintUsage() {
    std::println(
        "Usage: shader_baker -i <input_file> -e <entry_point> -o <output_file> [-I "
        "<include_path>...]");
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

    if (inputPath.empty() || outputPath.empty()) {
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
    auto resultOrError = [&]() {
        if (entryPoint.empty()) {
            return compiler.Compile(inputPath.string());
        } else {
            return compiler.Compile(inputPath.string(), std::string(entryPoint));
        }
    }();
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
        return 0;
    } else {
        return 1;
    }
}