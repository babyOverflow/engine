
#include "PipelineManager.h"
#include "render/graph/IRenderPass.h"

namespace core::render {
DepthStencilStateManager::DepthStencilStateManager() {
    // Register a default depth stencil state at index 0, which can be used when a pass doesn't
    // specify any depth stencil state.
    m_depthStencilStates.resize(2);

    m_depthStencilStates[kNullStateID] = {};

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
      m_vertexLayoutManager(vertexLayoutManager),
      m_passManager(passManager) {
    m_globalBindGroupLayout = m_layoutCache->GetBindGroupLayout(globalBindGroupLayoutDesc);
}

Handle PipelineManager::GetOrCreatePipeline(const PipelineConfig& config) {
    PipelineKey key{};
    key.bits.shaderId = config.shader.handle.index;
    key.bits.layoutId = config.layoutId;
    key.bits.blendId =
        static_cast<uint64_t>(config.blendMode);      // TODO: support blend mode in material
    key.bits.depthStencilId = config.depthStencilId;  // TODO: support depth
                                                      // state in material
    key.bits.passId = config.passId;
    key.bits.topology = static_cast<uint64_t>(wgpu::PrimitiveTopology::TriangleList);
    key.bits.cullMode = static_cast<uint64_t>(config.cullMode);
    key.bits.frontFace = static_cast<uint64_t>(wgpu::FrontFace::CCW);

    auto it = m_pipelineIDCache.find(key.hash);
    if (it != m_pipelineIDCache.end()) {
        return it->second;
    }
    auto vertexState = m_vertexLayoutManager->GetAllVertexStates()[key.bits.layoutId];

    std::vector<wgpu::VertexBufferLayout> vertexLayouts;

    auto entryOpt = config.shader->GetReflection().GetEntryPointOffsetByName("vertexMain");
    assert(entryOpt.has_value() && "Invalid vertex shader entry point name in pass signature");

    uint32_t entryIdx = entryOpt.value();
    std::span<const ShaderReflection::Parameter> inputs =
        config.shader->GetReflection().GetEntryIO(entryIdx);

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
        bindGroupLayouts.push_back(config.shader->GetBindGroupLayout(i));
    }

    wgpu::PipelineLayoutDescriptor pipelineLayoutDesc{
        .bindGroupLayoutCount = bindGroupLayouts.size(),
        .bindGroupLayouts = bindGroupLayouts.data()};

    const auto& renderPipelineLayout = m_layoutCache->GetPipelineLayout(pipelineLayoutDesc);

    std::vector<wgpu::ColorTargetState> targets;
    targets.reserve(config.targetState->colorTargetFormats.size());
    for (const wgpu::TextureFormat colorTargetFormat : config.targetState->colorTargetFormats) {
        targets.push_back(wgpu::ColorTargetState{.format = colorTargetFormat,
                                                 .blend = &wgx::BlendState::kReplace,
                                                 .writeMask = wgpu::ColorWriteMask::All});
    }

    std::string fragmentEntry("fragmentMain");
    wgpu::FragmentState fragment = wgpu::FragmentState{
        .module = config.shader->GetShaderModule(),
        .entryPoint = fragmentEntry.c_str(),
        .targetCount = targets.size(),
        .targets = targets.data(),
    };

    std::string passName = m_passManager->GetPassName(config.passId);

    wgpu::PrimitiveState primitiveState{
        .topology = static_cast<wgpu::PrimitiveTopology>(key.bits.topology),
        .frontFace = static_cast<wgpu::FrontFace>(key.bits.frontFace),
        .cullMode = static_cast<wgpu::CullMode>(key.bits.cullMode),

    };
    const wgpu::DepthStencilState* depthStencilState =
        m_depthStencilStateManager.GetDepthStencilState(key.bits.depthStencilId);
    wgpu::RenderPipeline renderPipeline =
        m_device->CreateRenderPipeline(wgpu::RenderPipelineDescriptor{
            .label = passName.c_str(),
            .layout = renderPipelineLayout,
            .vertex = wgpu::VertexState{.module = config.shader->GetShaderModule(),
                                        .entryPoint = "vertexMain",
                                        .bufferCount = vertexLayouts.size(),
                                        .buffers = vertexLayouts.data()},
            .primitive = primitiveState,
            .depthStencil = depthStencilState,
            .fragment = &fragment,
        });

    Handle handle = m_pipelinePool.Attach(std::move(renderPipeline));
    m_pipelineIDCache[key.hash] = handle;
    return handle;
}

}  // namespace core::render
