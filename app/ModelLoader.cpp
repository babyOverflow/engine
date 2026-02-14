#include "ModelLoader.h"
#include <import/GLTFImporter.h>

std::expected<core::Handle, core::Error> loader::GLTFLoader::LoadModel(
    const std::string& path) {

    if (m_modelCache.find(path) != m_modelCache.end()) {
        return m_modelCache[path];
    }
    auto resultOrError = core::importer::GLTFImporter::ImportFromFile(path);
    if (!resultOrError.has_value()) {
        return std::unexpected(resultOrError.error());
    }
    const auto& result = resultOrError.value();

    for (const core::importer::TextureResult& textureResult : result.textures) {
        core::Handle handle = m_textureManger->LoadTexture(textureResult);
    }

    for (const auto& meshResult : result.meshes) {
        core::Handle handle = m_meshManager->LoadMesh(meshResult);
    }

    core::render::Model model;
    for (const auto& node : result.modelAsset.nodes) {
        core::Handle meshHandle = m_meshManager->GetMeshHandle(node.meshId);
        if (!meshHandle.IsValid()) {
            continue;
        }

        auto meshView = m_meshManager->GetMesh(meshHandle);

        for (size_t i = 0; i < meshView->submeshInfos.size(); ++i) {
            core::render::RenderUnit renderUnit;
            renderUnit.meshHandle = meshHandle;
            renderUnit.subMeshIndex = static_cast<uint32_t>(i);
            if (i < node.materialIds.size()) {
                // TODO! load material and set to render unit
            }
            renderUnit.modelMatrix = node.localMatrix;
            model.renderUnits.push_back(std::move(renderUnit));
        }
    }
    
    core::Handle modelHandle = m_assetManager->StoreModel(std::move(model));
    m_modelCache[path] = modelHandle;
    return modelHandle;
    
}
