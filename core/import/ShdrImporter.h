#pragma once

#include <Common.h>
#include "Importer.h"
#include "ShaderAssetFormat.h"
#include "render/ShaderAsset.h"
namespace core::render {
class ShaderReflectionData;
}
namespace core::importer {

struct ShaderBlob {
    wgpu::ShaderModule shaderModule;
    core::render::ShaderReflectionData reflection;
};

class ShdrImporter {
  public:
    static std::expected<ShaderBlob, Error> ShdrImport(core::render::Device* device,
                                                       const std::string& shaderPath);
    static std::expected<ShaderBlob, Error> ShdrImport(core::render::Device* device,
                                                       const core::ShaderAssetFormat& shdr);
};
}  // namespace core::importer