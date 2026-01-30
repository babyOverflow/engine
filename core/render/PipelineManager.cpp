
#include "PipelineManager.h"
#include "Mesh.h"
#include "render/util.h"

namespace core::render {
PipelineManager::PipelineManager(Device* device,
                                 LayoutCache* layoutCache,
                                 wgpu::BindGroupLayoutDescriptor& globalBindGroupLayoutDesc)
    : m_device(device), m_layoutCache(layoutCache) {
    m_globalBindGroupLayout = m_layoutCache->GetBindGroupLayout(globalBindGroupLayoutDesc);
}

const GpuRenderPipeline* PipelineManager::GetRenderPipeline(const PipelineDesc& desc) {
    if (m_pipelineCache.contains(desc)) {
        return &m_pipelineCache.at(desc);
    }

    std::vector<wgpu::BindGroupLayout> bindGroupLayouts{m_globalBindGroupLayout};
    for (uint32_t i = 1; i < 4; ++i) {
        bindGroupLayouts.push_back(desc.shaderAsset->GetBindGroupLayout(i));
    }

    wgpu::PipelineLayoutDescriptor pipelineLayoutDesc{
        .bindGroupLayoutCount = bindGroupLayouts.size(),
        .bindGroupLayouts = bindGroupLayouts.data()};

    const auto& renderPipelineLayout = m_layoutCache->GetPipelineLayout(pipelineLayoutDesc);

    wgpu::VertexBufferLayout bufferLayout = MapVertexDesc(desc.vertexType);
    std::array<wgpu::VertexBufferLayout, 1> vertexBufferLayouts{
        bufferLayout,
    };

    wgpu::ColorTargetState targets{.format = m_device->GetSurfaceConfig().format,
                                   .blend = &desc.blendState,
                                   .writeMask = wgpu::ColorWriteMask::All};
    wgpu::FragmentState fragment =
        wgpu::FragmentState{.module = desc.shaderAsset->GetFragmentModule(),
                            .entryPoint = "fragmentMain",
                            .targetCount = 1,
                            .targets = &targets};
    GpuRenderPipeline renderPipeline =
        m_device->CreateRenderPipeline(wgpu::RenderPipelineDescriptor{
            .layout = renderPipelineLayout,
            .vertex = wgpu::VertexState{.module = desc.shaderAsset->GetVertexModule(),
                                        .entryPoint = "vertexMain",
                                        .bufferCount = vertexBufferLayouts.size(),
                                        .buffers = vertexBufferLayouts.data()},
            .primitive = desc.primitive,
            .fragment = &fragment,
        });

    m_pipelineCache[desc] = std::move(renderPipeline);
    return &m_pipelineCache[desc];
}
}  // namespace core::render
