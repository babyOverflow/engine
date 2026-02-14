
#include "AssetManager.h"
#include "ShaderAssetFormat.h"

#include "import/GLTFImporter.h"
#include "render/ShaderSystem.h"
#include "render/util.h"

#include <slang.h>

const std::string kGltfPosition = "POSITION";
const std::string kGltfTexCoord0 = "TEXCOORD_0";
const std::string kGltfNormal = "NORMAL";
const std::string kGltfTangent = "TANGENT";

using namespace slang;

namespace core {
AssetManager AssetManager::Create(render::Device* device) {
    return AssetManager(device);
}

AssetManager::AssetManager(render::Device* device) : m_device(device) {}

Handle AssetManager::LoadModel(std::string filePath) {
    //auto model = importer::GLTFImporter::ImportFromFile(this, m_device, filePath);
    //if (!model.has_value()) {
    //    // TODO! expected error handling
    //    return Handle();
    //}
    //return model.value();
    return Handle();
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


}  // namespace core