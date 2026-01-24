#pragma once

#include <expected>

#include "Common.h"
#include "ResourcePool.h"
#include "render/Mesh.h"

namespace core::render {
class Device;
}

namespace core {
class AssetManager;
}

namespace core::import {
class GLTFImporter {
  public:
    static std::expected<Handle, Error> ImportFromFile(AssetManager* assetManager,
                                                       const render::Device* device,
                                                       const std::string& filePath);
};
}  // namespace core::import