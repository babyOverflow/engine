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
    header.parameterCount = static_cast<uint16_t>(result.parameters.size());
    header.bindingCount = static_cast<uint16_t>(result.bindings.size());
    header.variableCount = static_cast<uint16_t>(result.variables.size());
    header.entryPointCount = static_cast<uint16_t>(result.entryPoints.size());
    header.shaderSize = static_cast<uint32_t>(result.sourceBlob.size());
    header.indexCount = static_cast<uint16_t>(result.indices.size());
    file.write(reinterpret_cast<const char*>(&header), sizeof(sa::Header));

    if (!result.parameters.empty()) {
        header.parameterOffset = file.tellp();
        file.write(reinterpret_cast<const char*>(result.parameters.data()),
                   sizeof(sa::ShaderParameter) * result.parameters.size());
    }
    if (!result.bindings.empty()) {
        header.bindingOffset = file.tellp();
        file.write(reinterpret_cast<const char*>(result.bindings.data()),
                   sizeof(sa::Binding) * result.bindings.size());
    }
    if (!result.variables.empty()) {
        header.variableOffset = file.tellp();
        file.write(reinterpret_cast<const char*>(result.variables.data()),
                   sizeof(sa::Variable) * result.variables.size());
    }
    if (!result.entryPoints.empty()) {
        header.entryPointOffset = file.tellp();
        file.write(reinterpret_cast<const char*>(result.entryPoints.data()),
                   sizeof(sa::EntryPoint) * result.entryPoints.size());
    }
    if (!result.sourceBlob.empty()) {
        header.shaderOffset = file.tellp();
        file.write(reinterpret_cast<const char*>(result.sourceBlob.data()),
                   result.sourceBlob.size());
    }
    if (!result.indices.empty()) {
        header.indexOffset = file.tellp();
        file.write(reinterpret_cast<const char*>(result.indices.data()),
                        result.indices.size());
    }

    if (!result.nameTable.empty()) {
        header.nameTableOffset = file.tellp();
        uint32_t size = 0;
        for (const auto& name : result.nameTable) {
            std::string_view nameView = name;
            file.write(nameView.data(), nameView.size());
            file.put('\0');
            size += nameView.size() + 1;
        }
        header.nameTableSize = size;
    }


    file.seekp(0);
    file.write(reinterpret_cast<const char*>(&header), sizeof(sa::Header));

    file.close();
    return true;
}