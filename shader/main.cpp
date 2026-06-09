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
        "Usage: shader_baker -i <input_file> -t <template_file> -o <output_file> [-I "
        "<include_path>...]");
}

int main(int argc, char** argv) {
    if (argc < 7) {  // Minimum required flags: -i, -t, -o (plus their arguments + binary name = 7)
        PrintUsage();
        return 1;
    }
    fs::path inputPath;
    fs::path outputPath;
    fs::path templatePath;
    std::vector<fs::path> includePaths;

    for (int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];
        if (arg == "-i" && i + 1 < argc) {
            inputPath = argv[++i];
        } else if (arg == "-o" && i + 1 < argc) {
            outputPath = argv[++i];
        } else if (arg == "-t" && i + 1 < argc) {  // ◀ Parse entry template configuration flag
            templatePath = argv[++i];
        } else if (arg == "-I" && i + 1 < argc) {
            includePaths.push_back(argv[++i]);
        }
    }

    if (inputPath.empty() || outputPath.empty()) {
        std::println(stderr, "Error: Missing required arguments (-i and -o are mandatory).");
        PrintUsage();
        return 1;
    }

    // Initialize compiler descriptor with template path parameters
    SlangCompilerDesc desc{.paths = includePaths, .entryTemplatePaths = templatePath};

    auto compilerOrError = SlangCompiler::Create(desc);
    if (!compilerOrError.has_value()) {
        std::println(stderr, "Error! Failed to initialize compiler: {}",
                     compilerOrError.error().message);
        return 1;
    }
    SlangCompiler compiler = compilerOrError.value();

    auto resultOrError = [&]() {
        if (templatePath.empty()) {
            std::println("Baking Shader Asset: {} ...", inputPath.string());
            return compiler.Compile(inputPath.string());
        } else {
            std::println("Baking Pass Asset: {} using template {} ...", inputPath.string(),
                         templatePath.string());
            return compiler.CompilePass(inputPath.string());
        }
    }();

    if (!resultOrError.has_value()) {
        std::println(stderr, "Error! Compilation failed for {}: {}", inputPath.string(),
                     resultOrError.error().message);
        return 1;
    }

    const CompileResult compileResult = resultOrError.value();

    if (!compileResult.warning.empty()) {
        std::println("Warning!\n {}", compileResult.warning);
    }

    // Write baked binary payload containing compiled WGSL and matched identifiers
    if (WriteAssetToFile(outputPath, compileResult)) {
        if (compileResult.passNameIdx != core::ShaderAssetFormat::kInvalidIdx &&
            compileResult.materialNameIdx != core::ShaderAssetFormat::kInvalidIdx &&
            compileResult.passNameIdx < compileResult.nameTable.size() &&
            compileResult.materialNameIdx < compileResult.nameTable.size()) {
            std::println("Successfully baked pass [{}] with material [{}]: {}", 
                         compileResult.nameTable[compileResult.passNameIdx],
                         compileResult.nameTable[compileResult.materialNameIdx], 
                         outputPath.string());
        } else {
            std::println("Successfully baked shader asset: {}", outputPath.string());
        }
        return 0;
    } else {
        std::println(stderr, "Error: Failed to serialize baked asset output stream to disk.");
        return 1;
    }
}