#include <memory>

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

    std::vector<uint8_t> globalVertexStateIds;
    for (const auto& state : meshResult.meshAsset.states) {
        globalVertexStateIds.push_back(m_vertexLayoutManager->GetVertexStateID(state));
    }

    Mesh mesh{
        .vertexBuffer = vertexBuffer,
        .indexBuffer = indexBuffer,
        .meshAssetFormat = std::make_unique<MeshAssetFormat>(meshResult.meshAsset),
        .globalVertexStateIds = std::move(globalVertexStateIds),
    };
    Handle handle = m_assetManager->StoreMesh(std::move(mesh));
    m_meshCache[meshResult.assetPath] = handle;
    return handle;
}
