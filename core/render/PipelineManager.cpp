
#include "PipelineManager.h"
#include "Mesh.h"
#include "render/util.h"

namespace core::render {
PipelineManager::PipelineManager(Device* device,
                                 wgpu::BindGroupLayoutDescriptor& globalBindGroupLayoutDesc)
    : m_device(device), m_layoutCache(LayoutCache(device)) {
    m_globalBindGroupLayout = m_layoutCache.GetBindGroupLayout(globalBindGroupLayoutDesc);
}

const GpuRenderPipeline* PipelineManager::GetRenderPipeline(const PipelineDesc& desc) {
    if (m_pipelineCache.contains(desc)) {
        return &m_pipelineCache.at(desc);
    }

    const auto& vertexBindingInfo = desc.vertexShader->GetBindGroupEntries();
    const auto& fragmantBindingInfo = desc.fragmentShader->GetBindGroupEntries();
    const auto bindingInfo =
        util::WgpuShaderBindingLayoutInfo::MergeVisibility(vertexBindingInfo, fragmantBindingInfo);

    std::vector<wgpu::BindGroupLayout> bindGroupLayouts{m_globalBindGroupLayout};
    for (uint32_t i = 1; i < 4; ++i) {
        const auto entries = bindingInfo.GetGroup(i);

        wgpu::BindGroupLayoutDescriptor bindGroupLayoutDesc{
            .entryCount = entries.size(),
            .entries = entries.data(),
        };
        const wgpu::BindGroupLayout bindGroupLayout =
            m_layoutCache.GetBindGroupLayout(bindGroupLayoutDesc);
        bindGroupLayouts.push_back(bindGroupLayout);
    }

    wgpu::PipelineLayoutDescriptor pipelineLayoutDesc{
        .bindGroupLayoutCount = bindGroupLayouts.size(),
        .bindGroupLayouts = bindGroupLayouts.data()};

    const auto& renderPipelineLayout = m_layoutCache.GetPipelineLayout(pipelineLayoutDesc);

    wgpu::VertexBufferLayout bufferLayout = MapVertexDesc(desc.vertexType);
    std::array<wgpu::VertexBufferLayout, 1> vertexBufferLayouts{
        bufferLayout,
    };

    wgpu::ColorTargetState targets{.format = m_device->GetSurfaceConfig().format,
                                   .blend = &desc.blendState,
                                   .writeMask = wgpu::ColorWriteMask::All};
    wgpu::FragmentState fragment = wgpu::FragmentState{.module = desc.fragmentShader->GetHandle(),
                                                       .entryPoint = "fragmentMain",
                                                       .targetCount = 1,
                                                       .targets = &targets};
    GpuRenderPipeline renderPipeline =
        m_device->CreateRenderPipeline(wgpu::RenderPipelineDescriptor{
            .layout = renderPipelineLayout,
            .vertex = wgpu::VertexState{.module = desc.vertexShader->GetHandle(),
                                        .entryPoint = "vertexMain",
                                        .bufferCount = vertexBufferLayouts.size(),
                                        .buffers = vertexBufferLayouts.data()},
            .primitive = desc.primitive,
            .fragment = &fragment,
        });

    m_pipelineCache[desc] = std::move(renderPipeline);
    return &m_pipelineCache[desc];
}

wgpu::BindGroupLayout PipelineManager::GetBindGroupLayout(
    wgpu::BindGroupLayoutDescriptor& bindGroupLayoutDesc) {
    return m_layoutCache.GetBindGroupLayout(bindGroupLayoutDesc);
}
}  // namespace core::render
