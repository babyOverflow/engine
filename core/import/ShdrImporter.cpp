#include "ShdrImporter.h"
#include "render/LayoutCache.h"
#include "render/ShaderAsset.h"
#include "util/Load.h"

namespace core::importer {

std::expected<ShaderImportResult, Error> core::importer::ShdrImporter::ShdrImport(
    const std::string& shaderPath) {
    const auto blobOrError = util::ReadFileToByte(shaderPath)
                                 .and_then(ShaderAssetFormat::LoadFromMemory)
                                 .and_then(ShdrImporter::ShdrConvert);
    if (!blobOrError.has_value()) {
        return std::unexpected(Error::Parse(""));
    }
    return ShaderImportResult{
        blobOrError.value(),
        AssetPath{shaderPath},
    };
}

std::expected<ShaderBlob, Error> ShdrImporter::ShdrConvert(const core::ShaderAssetFormat& shdr) {
    using Fmt = ShaderAssetFormat;

    const std::vector<Fmt::Binding>& bindings = shdr.bindings;

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
            .id = binding.id,
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
        .shaderCode = std::move(shdr.code),
        .reflection = std::move(reflection),
    };
}

}  // namespace core::importer