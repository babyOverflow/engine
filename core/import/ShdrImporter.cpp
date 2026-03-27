#include "ShdrImporter.h"
#include "render/LayoutCache.h"
#include "render/ShaderAsset.h"
#include "util/Load.h"

namespace core::importer {

std::expected<ShaderImportResult, Error> core::importer::ShdrImporter::ShdrImport(
    const std::string& shaderPath) {
    const auto blobOrError =
        util::ReadFileToByte(shaderPath).and_then(ShaderAssetFormat::LoadFromMemory);
    if (!blobOrError.has_value()) {
        return std::unexpected(Error::Parse(""));
    }
    return ShaderImportResult{
        blobOrError.value(),
        AssetPath{shaderPath},
    };
}



}  // namespace core::importer