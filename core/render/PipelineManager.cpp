#include <ranges>

#include "Mesh.h"
#include "PipelineManager.h"
#include "render/util.h"

namespace core::render {

static constexpr wgpu::VertexFormat MapFormat(MeshAssetFormat::VertexFormat format) {
    switch (format) {
        case MeshAssetFormat::VertexFormat::Float32:
            return wgpu::VertexFormat::Float32;
        case MeshAssetFormat::VertexFormat::Float32x2:
            return wgpu::VertexFormat::Float32x2;
        case MeshAssetFormat::VertexFormat::Float32x3:
            return wgpu::VertexFormat::Float32x3;
        case MeshAssetFormat::VertexFormat::Float32x4:
            return wgpu::VertexFormat::Float32x4;
        default:
            assert(false && "Unhandled VertexFormat provided");
            std::unreachable();
    }
}

static constexpr wgpu::VertexStepMode MapStepMode(MeshAssetFormat::StepMode stepMode) {
    switch (stepMode) {
        case MeshAssetFormat::StepMode::Vertex:
            return wgpu::VertexStepMode::Vertex;
        case MeshAssetFormat::StepMode::Instance:
            return wgpu::VertexStepMode::Instance;
        default:
            return wgpu::VertexStepMode::Undefined;
    }
}

PipelineManager::PipelineManager(Device* device,
                                 LayoutCache* layoutCache,
                                 wgpu::BindGroupLayoutDescriptor& globalBindGroupLayoutDesc)
    : m_device(device), m_layoutCache(layoutCache) {
    m_globalBindGroupLayout = m_layoutCache->GetBindGroupLayout(globalBindGroupLayoutDesc);
}

wgpu::RenderPipeline PipelineManager::GetRenderPipeline(const PipelineDesc& desc) {
    if (m_pipelineCache.contains(desc)) {
        return m_pipelineCache.at(desc);
    }

    std::vector<wgpu::BindGroupLayout> bindGroupLayouts{
        m_globalBindGroupLayout,
    };
    for (uint32_t i = 1; i < 4; ++i) {
        bindGroupLayouts.push_back(desc.shaderAsset->GetBindGroupLayout(i));
    }

    wgpu::PipelineLayoutDescriptor pipelineLayoutDesc{
        .bindGroupLayoutCount = bindGroupLayouts.size(),
        .bindGroupLayouts = bindGroupLayouts.data()};

    const auto& renderPipelineLayout = m_layoutCache->GetPipelineLayout(pipelineLayoutDesc);

    std::span<const ShaderReflection::Parameter> inputs =
        desc.shaderAsset->GetReflection().GetEntryInput(desc.vertexEntry);

    std::vector<wgpu::VertexBufferLayout> vertexLayouts;

    for (const auto& slot : desc.vertexState.bufferSlots) {
        std::vector<wgx::VertexAttribute> activeAttributes;

        for (uint32_t i = 0; i < slot.attributeCount; ++i) {
            auto& meshAtt = slot.attributes[i];
            auto it = std::ranges::find_if(inputs, [&](const ShaderReflection::Parameter& param) {
                return param.semantic == meshAtt.semantic;
            });

            if (it != inputs.end()) {
                activeAttributes.push_back(wgx::VertexAttribute{.format = MapFormat(meshAtt.format),
                                                                .offset = meshAtt.offset,
                                                                .shaderLocation = it->location});
            }
        }

        if (activeAttributes.empty()) {
            continue;
        }

        Handle layoutHandle = m_vertexLayoutManager.GetVertexLayout(
            wgx::VertexBufferLayout{.stepMode = MapStepMode(slot.stepMode),
                                    .arrayStride = slot.stride,
                                    .attributes = std::move(activeAttributes)});

        vertexLayouts.push_back(m_vertexLayoutManager.GetVertexLayout(layoutHandle)->GetLayout());
    }

    wgpu::ColorTargetState targets{.format = m_device->GetSurfaceConfig().format,
                                   .blend = &desc.blendState,
                                   .writeMask = wgpu::ColorWriteMask::All};
    wgpu::FragmentState fragment =
        wgpu::FragmentState{.module = desc.shaderAsset->GetShaderModule(),
                            .entryPoint = desc.fragmentEntry.c_str(),
                            .targetCount = 1,
                            .targets = &targets};
    wgpu::RenderPipeline renderPipeline =
        m_device->CreateRenderPipeline(wgpu::RenderPipelineDescriptor{
            .layout = renderPipelineLayout,
            .vertex = wgpu::VertexState{.module = desc.shaderAsset->GetShaderModule(),
                                        .entryPoint = desc.vertexEntry.c_str(),
                                        .bufferCount = vertexLayouts.size(),
                                        .buffers = vertexLayouts.data()},
            .primitive = desc.primitive,
            .fragment = &fragment,
        });

    m_pipelineCache[desc] = std::move(renderPipeline);
    return m_pipelineCache[desc];
}
}  // namespace core::render
