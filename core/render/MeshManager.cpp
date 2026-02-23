#include "MeshManager.h"

core::Handle core::render::MeshManager::LoadMesh(const importer::MeshResult& meshResult) {
    if (m_meshCache.find(meshResult.assetPath) != m_meshCache.end()) {
        return m_meshCache[meshResult.assetPath];
    }

    const MeshAssetFormat& meshAssetFormat = meshResult.meshAsset;

    wgpu::Buffer vertexBuffer = m_device->CreateBufferFromData(
        meshAssetFormat.vertexData.data(), meshAssetFormat.vertexData.size(),
        wgpu::BufferUsage::Vertex | wgpu::BufferUsage::CopyDst);
    wgpu::Buffer indexBuffer = m_device->CreateBufferFromData(
        meshAssetFormat.indexData.data(), meshAssetFormat.indexData.size() * sizeof(uint32_t),
        wgpu::BufferUsage::Index | wgpu::BufferUsage::CopyDst);

    std::vector<SubMeshInfo> subMeshInfos;
    subMeshInfos.reserve(meshAssetFormat.subMeshes.size());
    for (const auto& subMeshAsset : meshAssetFormat.subMeshes) {
        SubMeshInfo subMeshInfo;
        subMeshInfo.indexCount = subMeshAsset.indexCount;
        subMeshInfo.indexOffset = subMeshAsset.baseIndexLocation;
        subMeshInfo.vertexOffset = subMeshAsset.baseVertexLocation;
        subMeshInfos.push_back(subMeshInfo);
    }

    Mesh mesh{
        .vertexBuffer = vertexBuffer,
        .indexBuffer = indexBuffer,
        .submeshInfos = std::move(subMeshInfos),
    };
    Handle handle = m_assetManager->StoreMesh(std::move(mesh));
    m_meshCache[meshResult.assetPath] = handle;
    return handle;
}
