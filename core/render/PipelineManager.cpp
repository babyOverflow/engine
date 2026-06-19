#include <ranges>

#include "IRenderPass.h"
#include "Mesh.h"
#include "PipelineManager.h"
#include "render/util.h"

namespace core::render {
DepthStencilStateManager::DepthStencilStateManager() {
    // Register a default depth stencil state at index 0, which can be used when a pass doesn't
    // specify any depth stencil state.
    m_depthStencilStates.resize(2);

    wgpu::DepthStencilState defaultState{};
    defaultState.format = wgpu::TextureFormat::Undefined;
    defaultState.depthWriteEnabled = false;
    defaultState.depthCompare = wgpu::CompareFunction::Always;
    m_depthStencilStates[kDefaultStateID] = defaultState;

    wgpu::DepthStencilState defaultDepthState{};
    defaultDepthState.format = wgpu::TextureFormat::Depth24PlusStencil8;
    defaultDepthState.depthWriteEnabled = true;
    defaultDepthState.depthCompare = wgpu::CompareFunction::Less;
    m_depthStencilStates[kDefaultDepthStateID] = defaultDepthState;
}

uint64_t DepthStencilStateManager::RegisterDepthStencilState(wgpu::DepthStencilState& state) {
    auto it = std::ranges::find_if(m_depthStencilStates, [&](const wgpu::DepthStencilState& s) {
        return std::memcmp(&s, &state, sizeof(wgpu::DepthStencilState)) == 0;
    });
    if (it != m_depthStencilStates.end()) {
        return std::distance(m_depthStencilStates.begin(), it);
    }
    m_depthStencilStates.push_back(state);
    return m_depthStencilStates.size() - 1;
}

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
                                 PassManager* passManager,
                                 VertexLayoutManager* vertexLayoutManager,
                                 wgpu::BindGroupLayoutDescriptor& globalBindGroupLayoutDesc)
    : m_device(device),
      m_layoutCache(layoutCache),
      m_passManager(passManager),
      m_vertexLayoutManager(vertexLayoutManager) {
    m_globalBindGroupLayout = m_layoutCache->GetBindGroupLayout(globalBindGroupLayoutDesc);
}

Handle PipelineManager::GetPipelineID(PipelineKey key, AssetRegistry assetRegistry) {
    auto it = m_pipelineIDCache.find(key.hash);
    if (it != m_pipelineIDCache.end()) {
        return it->second;
    }
    auto pass = m_passManager->CreatePass(key.bits.passId);
    auto vertexState = m_vertexLayoutManager->GetAllVertexStates()[key.bits.layoutId];

    std::vector<wgpu::VertexBufferLayout> vertexLayouts;

    const ShaderAsset& shaderAsset = assetRegistry.shaders[key.bits.shaderId];
    auto entryOpt =
        shaderAsset.GetReflection().GetEntryPointOffsetByName("vertexMain");
    assert(entryOpt.has_value() && "Invalid vertex shader entry point name in pass signature");

    uint32_t entryIdx = entryOpt.value();
    std::span<const ShaderReflection::Parameter> inputs =
        shaderAsset.GetReflection().GetEntryInput(entryIdx);

    for (const auto& slot : vertexState.bufferSlots) {
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

        Handle layoutHandle = m_vertexLayoutManager->GetVertexLayout(
            wgx::VertexBufferLayout{.stepMode = MapStepMode(slot.stepMode),
                                    .arrayStride = slot.stride,
                                    .attributes = std::move(activeAttributes)});

        vertexLayouts.push_back(m_vertexLayoutManager->GetVertexLayout(layoutHandle)->GetLayout());
    }

    std::vector<wgpu::BindGroupLayout> bindGroupLayouts{
        m_globalBindGroupLayout,
    };
    for (uint32_t i = 1; i < 4; ++i) {
        bindGroupLayouts.push_back(shaderAsset.GetBindGroupLayout(i));
    }

    wgpu::PipelineLayoutDescriptor pipelineLayoutDesc{
        .bindGroupLayoutCount = bindGroupLayouts.size(),
        .bindGroupLayouts = bindGroupLayouts.data()};

    const auto& renderPipelineLayout = m_layoutCache->GetPipelineLayout(pipelineLayoutDesc);

    wgpu::ColorTargetState targets{
        .format = m_device->GetSurfaceConfig().format,
        .blend = &wgx::BlendState::kReplace,  // TODO!(Sunghyun): Support blend state variations in
                                              // PipelineKey and PassSignature
        .writeMask = wgpu::ColorWriteMask::All};
    std::string fragmentEntry("fragmentMain");
    wgpu::FragmentState fragment = wgpu::FragmentState{.module = shaderAsset.GetShaderModule(),
                                                       .entryPoint = fragmentEntry.c_str(),
                                                       .targetCount = 1,
                                                       .targets = &targets};

    wgpu::DepthStencilState depthStencilState =
        m_depthStencilStateManager.GetDepthStencilState(key.bits.depthStencilId);
    wgpu::RenderPipeline renderPipeline =
        m_device->CreateRenderPipeline(wgpu::RenderPipelineDescriptor{
            .layout = renderPipelineLayout,
            .vertex = wgpu::VertexState{.module = shaderAsset.GetShaderModule(),
                                        .entryPoint = "vertexMain",
                                        .bufferCount = vertexLayouts.size(),
                                        .buffers = vertexLayouts.data()},
            .primitive =
                wgx::PrimitiveState::kDefault,  // TODO!(Sunghyun): Support primitive state
                                                // variations in PipelineKey and PassSignature
            .depthStencil = &depthStencilState,
            .fragment = &fragment,
        });

    Handle handle = m_pipelinePool.Attach(std::move(renderPipeline));
    m_pipelineIDCache[key.hash] = handle;
    return handle;
}

}  // namespace core::render
