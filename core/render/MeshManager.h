#pragma once
#include "import/Importer.h"
#include "AssetManager.h"
#include "MeshAssetFormat.h"
#include "render.h"

namespace core::render {

class MeshManager {
  public:
    MeshManager() = default;
    ~MeshManager() = default;

    MeshManager(Device* device, AssetManager* assetManager)
        : m_device(device), m_assetManager(assetManager) {}

    Handle LoadMesh(const importer::MeshResult& meshAssetFormat);
    Handle GetMeshHandle(const AssetPath& assetPath) const {
        auto it = m_meshCache.find(assetPath);
        if (it != m_meshCache.end()) {
            return it->second;
        }
        return Handle();
    }
    AssetView<Mesh> GetMesh(Handle handle) {
        return {m_assetManager->GetMesh(handle).Get(), handle};
    }
  private:
    Device* m_device = nullptr;
    AssetManager* m_assetManager = nullptr;

    std::unordered_map<AssetPath, Handle> m_meshCache;
};
}  // namespace core::render