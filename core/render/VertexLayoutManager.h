#pragma once
#include "ResourcePool.h"
#include "VertexLayout.h"
#include "wgx/types.h"

namespace core::render {

class VertexLayout {
    friend class VertexLayoutManager;

  public:
    wgpu::VertexBufferLayout GetLayout() { return m_layout; }

  private:
    VertexLayout() = default;
    wgpu::VertexBufferLayout m_layout;
    std::vector<wgpu::VertexAttribute> m_attributes;
};

class VertexLayoutManager {
  public:
    VertexLayoutManager() = default;

    core::Handle GetVertexLayout(const wgx::VertexBufferLayout& layout);
    AssetView<VertexLayout> GetVertexLayout(core::Handle handle) { return {m_vertexLayouts.Get(handle), handle}; }

    struct VertexAttributeKey {
        wgpu::VertexFormat format = {};
        uint64_t offset;
        uint32_t shaderLocation;
    };

  private:

    std::vector<std::pair<wgx::VertexBufferLayout, Handle>> m_layoutCache;
    ResourcePool<VertexLayout> m_vertexLayouts;
};
}  // namespace core::render