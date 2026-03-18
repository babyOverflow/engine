#include "ShaderAssetFormat.h"

namespace core {

std::expected<ShaderAssetFormat, Error> core::ShaderAssetFormat::LoadFromMemory(
    std::span<const uint8_t> memory) {
    using Header = ShaderAssetFormat::Header;
    using Binding = ShaderAssetFormat::Binding;

    constexpr size_t HeaderSize = sizeof(Header);
    const uint8_t* ptr = memory.data();

    ShaderAssetFormat shaderAsset;

    if (memory.size() < sizeof(ShaderAssetFormat::Header)) {
        return std::unexpected(Error::Parse("Buffer too small for header"));
    }

    std::memcpy(&shaderAsset.header, ptr, HeaderSize);

    const Header& header = shaderAsset.header;

    if (header.magicNumber != ShaderAssetFormat::SHADER_ASSET_MAGIC) {
        return std::unexpected(Error::AssetParsing(
            std::format("Invalid Magic Number: expected {:#x}, but got {:#x}",
                        ShaderAssetFormat::SHADER_ASSET_MAGIC, header.magicNumber)));
    }
    if (header.version != ShaderAssetFormat::SHADER_ASSET_VERSION) {
        return std::unexpected(Error::AssetParsing(
            std::format("Unsupported Version: version {} is not supported (current: {}).",
                        header.version, ShaderAssetFormat::SHADER_ASSET_VERSION)));
    }
    const size_t parametersOffset = header.parameterOffset;
    const size_t bindingsOffset = header.bindingOffset;
    const size_t variablesOffset = header.variableOffset;
    const size_t entryPointsOffset = header.entryPointOffset;
    const size_t codeOffset = header.shaderOffset;
    const size_t nameTableOffset = header.nameTableOffset;
    const size_t totalSize = nameTableOffset + header.nameTableSize;

    if (memory.size() < totalSize) {
        return std::unexpected(Error::AssetParsing(
            "Corrupted Asset: actual data size does not match header description"));
    }

    if (header.parameterCount > 0) {
        shaderAsset.parameters.resize(header.parameterCount);
        std::memcpy(shaderAsset.parameters.data(), ptr + parametersOffset,
                    sizeof(ShaderParameter) * header.parameterCount);
    }

    if (header.bindingCount > 0) {
        shaderAsset.bindings.resize(header.bindingCount);
        std::memcpy(shaderAsset.bindings.data(), ptr + bindingsOffset,
                    sizeof(Binding) * header.bindingCount);
    }

    if (header.variableCount > 0) {
        shaderAsset.variables.resize(header.variableCount);
        std::memcpy(shaderAsset.variables.data(), ptr + variablesOffset,
                    sizeof(Variable) * header.shaderSize);
    }

    if (header.entryPointCount > 0) {
        shaderAsset.entryPoints.resize(header.entryPointCount);
        std::memcpy(shaderAsset.entryPoints.data(), ptr + entryPointsOffset,
                    sizeof(EntryPoint) * header.entryPointCount);
    }

    if (header.shaderSize > 0) {
        shaderAsset.code.resize(header.shaderSize);
        std::memcpy(shaderAsset.code.data(), ptr + codeOffset, header.shaderSize);
    }

    if (header.indexCount > 0) {
        shaderAsset.indices.resize(header.indexCount);
        std::memcpy(shaderAsset.indices.data(), ptr + header.indexOffset,
                    header.indexCount * sizeof(uint32_t));
    }

    shaderAsset.tokens;
    if (header.nameTableSize > 0) {
        shaderAsset.nameTableData.resize(header.nameTableSize);
        std::memcpy(shaderAsset.nameTableData.data(), ptr + nameTableOffset, header.nameTableSize);
        std::string_view view(shaderAsset.nameTableData);

        size_t start = 0;
        while (start < view.size()) {
            size_t end = view.find('\0', start);

            if (end == std::string_view::npos) {
                end = view.size();
            }

            if (end > start) {
                shaderAsset.tokens.push_back(view.substr(start, end - start));
            }

            start = end + 1;
        }
    }

    return shaderAsset;
}

core::ShaderAssetFormat::Resource core::ShaderAssetFormat::Resource::Buffer(uint32_t size) {
    using sa = core::ShaderAssetFormat;

    sa::Buffer buffer{size};
    return sa::Resource{.buffer = buffer};
}

core::ShaderAssetFormat::Shape core::ShaderAssetFormat::Shape::Vector(uint8_t count) {
    return ShaderAssetFormat::Shape{
        .vector = ShaderAssetFormat::Vector{count},
    };
}
core::ShaderAssetFormat::Shape core::ShaderAssetFormat::Shape::Matrix(uint8_t rows,
                                                                      uint8_t columns) {
    return ShaderAssetFormat::Shape{
        .matrix = ShaderAssetFormat::Matrix{rows, columns},
    };
}
}  // namespace core
