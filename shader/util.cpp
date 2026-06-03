#include "util.h"
#include <fstream>
#include <print>

namespace fs = std::filesystem;
using namespace slangCompiler;
using sa = core::ShaderAssetFormat;

bool WriteAssetToFile(const fs::path& outputPath, const CompileResult& result) {
    std::ofstream file(outputPath, std::ios::binary);
    if (!file.is_open()) {
        std::println(stderr, "Error: Failed to open output file: {}", outputPath.string());
        return false;
    }

    sa::Header header;
    // Initialize offsets to 0
    header.parameterOffset = 0;
    header.bindingOffset = 0;
    header.variableOffset = 0;
    header.entryPointOffset = 0;
    header.indexOffset = 0;
    header.nameTableOffset = 0;
    header.shaderOffset = 0;

    // First, write a dummy header to reserve space
    file.write(reinterpret_cast<const char*>(&header), sizeof(sa::Header));

    if (!result.parameters.empty()) {
        header.parameterOffset = static_cast<uint32_t>(file.tellp());
        header.parameterCount = static_cast<uint16_t>(result.parameters.size());
        file.write(reinterpret_cast<const char*>(result.parameters.data()),
                   sizeof(sa::ShaderParameter) * result.parameters.size());
    }

    if (!result.bindings.empty()) {
        header.bindingOffset = static_cast<uint32_t>(file.tellp());
        header.bindingCount = static_cast<uint16_t>(result.bindings.size());
        file.write(reinterpret_cast<const char*>(result.bindings.data()),
                   sizeof(sa::Binding) * result.bindings.size());
    }

    if (!result.variables.empty()) {
        header.variableOffset = static_cast<uint32_t>(file.tellp());
        header.variableCount = static_cast<uint16_t>(result.variables.size());
        file.write(reinterpret_cast<const char*>(result.variables.data()),
                   sizeof(sa::Variable) * result.variables.size());
    }

    if (!result.entryPoints.empty()) {
        header.entryPointOffset = static_cast<uint32_t>(file.tellp());
        header.entryPointCount = static_cast<uint16_t>(result.entryPoints.size());
        file.write(reinterpret_cast<const char*>(result.entryPoints.data()),
                   sizeof(sa::EntryPoint) * result.entryPoints.size());
    }

    if (!result.sourceBlob.empty()) {
        header.shaderOffset = static_cast<uint32_t>(file.tellp());
        header.shaderSize = static_cast<uint32_t>(result.sourceBlob.size());
        file.write(reinterpret_cast<const char*>(result.sourceBlob.data()),
                   result.sourceBlob.size());
    }

    if (!result.passes.empty()) {
        header.indexOffset = static_cast<uint32_t>(file.tellp());
        header.indexCount = static_cast<uint16_t>(result.passes.size());
        file.write(reinterpret_cast<const char*>(result.passes.data()),
                   sizeof(sa::Pass) * result.passes.size());
    }

    if (!result.nameTable.empty()) {
        header.nameTableOffset = static_cast<uint32_t>(file.tellp());
        uint32_t size = 0;
        for (const auto& name : result.nameTable) {
            file.write(name.c_str(), name.size() + 1); // Write including null terminator
            size += static_cast<uint32_t>(name.size() + 1);
        }
        header.nameTableSize = size;
    }

    // Finalize header and overwrite the dummy
    header.magicNumber = sa::SHADER_ASSET_MAGIC;
    header.version = sa::SHADER_ASSET_VERSION;

    file.seekp(0);
    file.write(reinterpret_cast<const char*>(&header), sizeof(sa::Header));

    file.close();
    return true;
}
