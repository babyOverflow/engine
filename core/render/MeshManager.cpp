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


    Mesh mesh{
        .vertexBuffer = vertexBuffer,
        .indexBuffer = indexBuffer,
        .meshAssetFormat = std::make_unique<MeshAssetFormat>( meshResult.meshAsset),
    };
    Handle handle = m_assetManager->StoreMesh(std::move(mesh));
    m_meshCache[meshResult.assetPath] = handle;
    return handle;
}
