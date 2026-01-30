#include "ShaderAssetFormat.h"

namespace core {

std::expected<ShaderAssetFormat, Error> core::ShaderAssetFormat::LoadFromMemory(
    std::span<const uint8_t> memory) {
    using Header = ShaderAssetFormat::Header;
    using Binding = ShaderAssetFormat::Binding;

    constexpr size_t HeaderSize = sizeof(Header);
    constexpr size_t BindingsOffset = HeaderSize;
    constexpr size_t BindingsStride = sizeof(Binding);
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

    const size_t CodeOffset = BindingsOffset + BindingsStride * header.bindingCount;
    const size_t totalSize = CodeOffset + header.shaderSize;
    if (memory.size() < totalSize) {
        return std::unexpected(Error::AssetParsing(
            "Corrupted Asset: actual data size does not match header description"));
    }

    if (header.bindingCount > 0) {
        shaderAsset.bindings.resize(header.bindingCount);

        std::memcpy(shaderAsset.bindings.data(), ptr + BindingsOffset,
                    BindingsStride * header.bindingCount);
    }

    if (header.shaderSize > 0) {
        shaderAsset.code.resize(header.shaderSize);
        std::memcpy(shaderAsset.code.data(), ptr + CodeOffset, header.shaderSize);
    }

    return shaderAsset;
}

core::ShaderAssetFormat::Resource core::ShaderAssetFormat::Resource::Buffer(uint32_t size) {
    using sa = core::ShaderAssetFormat;

    sa::Buffer buffer{size};
    return sa::Resource{.buffer = buffer};
}

}  // namespace core
