#pragma once
#include <cstdint>

#include <Common.h>
#include "Importer.h"
#include "ShaderAssetFormat.h"
#include "render/ShaderAsset.h"
namespace core::render {
class ShaderReflection;
}
namespace core::importer {



class ShdrImporter {
  public:
    static std::expected<ShaderImportResult, Error> ShdrImport(const std::string& shaderPath);
};
}  // namespace core::importer