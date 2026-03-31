#include "VertexLayoutManager.h"
#include <ranges>
#include <span>

core::Handle core::render::VertexLayoutManager::GetVertexLayout(
    const wgx::VertexBufferLayout& layout) {
    auto it = std::ranges::find_if(
        m_layoutCache, [layout](const std::pair<wgx::VertexBufferLayout, Handle>& pair) {
            return pair.first == layout;
        });
    if (it != m_layoutCache.end()) {
        return it->second;
    }

    VertexLayout newLayout;
    newLayout.m_attributes = layout.attributes | std::views::transform([](auto att) {
                                 return wgpu::VertexAttribute{
                                     .format = att.format,
                                     .offset = att.offset,
                                     .shaderLocation = att.shaderLocation,
                                 };
                             }) |
                             std::ranges::to<std::vector>();
    newLayout.m_layout.arrayStride = layout.arrayStride;
    newLayout.m_layout.stepMode = layout.stepMode;
    newLayout.m_layout.attributes = newLayout.m_attributes.data();
    newLayout.m_layout.attributeCount = newLayout.m_attributes.size();

    core::Handle handle = m_vertexLayouts.Attach(std::move(newLayout));
    m_layoutCache.push_back({layout, handle});
    return handle;
}
