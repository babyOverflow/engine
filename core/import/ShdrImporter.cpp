#include "ShdrImporter.h"
#include "render/LayoutCache.h"
#include "render/ShaderAsset.h"
#include "util/Load.h"

namespace core::importer {

std::expected<ShaderBlob, Error> core::importer::ShdrImporter::ShdrImport(
    core::render::Device* device,
    const std::string& shaderPath) {
    const auto shrdOrError =
        util::ReadFileToByte(shaderPath).and_then(ShaderAssetFormat::LoadFromMemory);
    if (!shrdOrError.has_value()) {
        return std::unexpected(Error::Parse(""));
    }
    return ShdrImport(device, shrdOrError.value());
}

std::expected<ShaderBlob, Error> ShdrImporter::ShdrImport(core::render::Device* device,
                                                          const core::ShaderAssetFormat& shdr) {
    using Fmt = ShaderAssetFormat;

    std::string_view wgslCode(reinterpret_cast<const char*>(shdr.code.data()), shdr.code.size());
    wgpu::ShaderModule shaderModule = device->CreateShaderModuleFromWGSL(wgslCode);

    const auto& bindings = shdr.bindings;

    render::ShaderReflectionData reflection;

    reflection.entryShaderStage = shdr.header.entryShaderStage;

    reflection.bindings.reserve(bindings.size());

    auto& groups = reflection.groups;

    for (size_t i = 0; i < bindings.size(); ++i) {
        const Fmt::Binding& binding = bindings[i];

        uint32_t size = 0;
        if (binding.resourceType == Fmt::ResourceType::UniformBuffer) {
            size = binding.resource.buffer.bufferSize;
        }
        render::BindingInfo bindingInfo{
            .set = binding.set,
            .binding = binding.binding,
            .resourceType = binding.resourceType,
            .visibility = binding.visibility,
            .resource = binding.resource,
        };

        reflection.bindings.push_back(bindingInfo);
        if (groups[binding.set].count == 0) {
            groups[binding.set].offset = reflection.bindings.size() - 1;
        }
        groups[binding.set].count++;
    }

    return ShaderBlob{
        .shaderModule = shaderModule,
        .reflection = std::move(reflection),
    };
}

}  // namespace core::importer