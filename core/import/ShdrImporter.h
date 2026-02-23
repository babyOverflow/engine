#pragma once
#include <cstdint>

#include <Common.h>
#include "Importer.h"
#include "ShaderAssetFormat.h"
#include "render/ShaderAsset.h"
namespace core::render {
class ShaderReflectionData;
}
namespace core::importer {



class ShdrImporter {
  public:
    static std::expected<ShaderImportResult, Error> ShdrImport(const std::string& shaderPath);
    static std::expected<ShaderBlob, Error> ShdrConvert(const core::ShaderAssetFormat& shdr);
};
}  // namespace core::importer