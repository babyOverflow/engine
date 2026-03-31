#pragma once

#include <array>

#include <MeshAssetFormat.h>

#include "VertexLayoutManager.h"
#include "render.h"

namespace core::render {

struct Mesh {
    wgpu::Buffer vertexBuffer;
    wgpu::Buffer indexBuffer;
    std::unique_ptr< MeshAssetFormat> meshAssetFormat;

    std::span<const MeshAssetFormat::SubMeshInfo> GetSubMeshInfors() const {
        return std::span<const MeshAssetFormat::SubMeshInfo>(meshAssetFormat->subMeshes.data(),
                                                       meshAssetFormat->subMeshes.size());
    }

    const MeshAssetFormat::SubMeshInfo& GetSubMeshInfors(uint32_t index) const {
        return meshAssetFormat->subMeshes[index];
    }

    const MeshAssetFormat::MeshVertexState& GetVertexState(uint32_t index) const {
        return meshAssetFormat->states[index];
    }

    std::span<const MeshAssetFormat::BufferRange> GetBufferRanges(uint32_t start, uint32_t count) const {
        return std::span<const MeshAssetFormat::BufferRange>(
            meshAssetFormat->bufferRanges.data() + start, count);
    }

};

}  // namespace core::render