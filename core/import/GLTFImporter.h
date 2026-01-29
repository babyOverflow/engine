#pragma once

#include "Common.h"
#include "Importer.h"
#include "ResourcePool.h"
#include "render/Mesh.h"

namespace core::importer {
class GLTFImporter {
  public:
    static std::expected<Handle, Error> ImportFromFile(AssetManager* assetManager,
                                                       render::MaterialSystem* materialSystem,
                                                       const render::Device* device,
                                                       const std::string& filePath);
};
}  // namespace core::importer