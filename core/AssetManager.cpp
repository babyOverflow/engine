#include "AssetManager.h"

#include <slang.h>

using namespace slang;

namespace core {
AssetManager AssetManager::Create() {
    return AssetManager();
}

Handle AssetManager::StoreModel(render::Model&& model) {
    return m_modelPool.Attach(std::move(model));
}

AssetView<render::Model> core::AssetManager::GetModel(Handle handle) {
    return {m_modelPool.Get(handle), handle};
}

AssetView<render::ShaderAsset> core::AssetManager::GetShaderAsset(Handle handle) {
    return {m_shaderPool.Get(handle), handle};
}

Handle AssetManager::StoreShaderAsset(render::ShaderAsset&& shader) {
    return m_shaderPool.Attach(std::move(shader));
}

Handle AssetManager::StoreTexture(render::Texture&& texture) {
    return m_texturePool.Attach(std::move(texture));
}

AssetView<render::Texture> AssetManager::GetTexture(Handle handle) {
    return {m_texturePool.Get(handle), handle};
}

Handle AssetManager::StoreMesh(render::Mesh&& mesh) {
    return m_meshPool.Attach(std::move(mesh));
}

AssetView<render::Mesh> AssetManager::GetMesh(Handle handle) {
    return {m_meshPool.Get(handle), handle};
}

Handle AssetManager::StoreMaterial(render::Material&& material) {
    return m_materialPool.Attach(std::move(material));
}

AssetView<render::Material> AssetManager::GetMaterial(Handle handle) {
    return {m_materialPool.Get(handle), handle};
}

const AssetRegistry AssetManager::GetRegistry() const {
    return AssetRegistry{
        m_modelPool.GetDataSpan(),   m_meshPool.GetDataSpan(),   m_materialPool.GetDataSpan(),
        m_texturePool.GetDataSpan(), m_shaderPool.GetDataSpan(),
    };
}

}  // namespace core
