#pragma once
#include <tiny_gltf.h>

#include "ResourcePool.h"
#include "render/resource/Material.h"
#include "render/resource/Mesh.h"
#include "render/resource/Model.h"
#include "render/resource/ShaderAsset.h"
#include "render/resource/Texture.h"

namespace core {

struct AssetRegistry {
    std::span<const render::Model> models;
    std::span<const render::Mesh> meshes;
    std::span<const render::Material> materials;
    std::span<const render::Texture> textures;
    std::span<const render::ShaderAsset> shaders;
};

class AssetManager {
  public:
    static AssetManager Create();

    Handle StoreModel(render::Model&& model);
    AssetView<render::Model> GetModel(Handle handle);

    Handle StoreTexture(render::Texture&& texture);
    AssetView<render::Texture> GetTexture(Handle handle);

    Handle StoreShaderAsset(render::ShaderAsset&& shader);
    AssetView<render::ShaderAsset> GetShaderAsset(Handle handle);

    Handle StoreMaterial(render::Material&& material);
    AssetView<render::Material> GetMaterial(Handle handle);

    Handle StoreMesh(render::Mesh&& mesh);
    AssetView<render::Mesh> GetMesh(Handle handle);

    const AssetRegistry GetRegistry() const;

  private:
    ResourcePool<render::ShaderAsset> m_shaderPool;
    ResourcePool<render::Texture> m_texturePool;
    ResourcePool<render::Material> m_materialPool;
    // TODO!(m_modelCache)
    ResourcePool<render::Model> m_modelPool;
    ResourcePool<render::Mesh> m_meshPool;
};
}  // namespace core
