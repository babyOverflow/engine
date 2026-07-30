
#include "SceneRenderer.h"
#include "render/backend/BindGroupManager.h"
#include "render/backend/PipelineManager.h"
#include "render/pass/DeferredGBufferPass.h"
#include "render/pass/DeferredLightingPass.h"
#include "render/pass/ForwardRenderPass.h"

void core::render::SceneCuller::ExtractRenderQueue(
    const Scene& scene,
    std::span<uint32_t> passes,
    AssetManager* assetManager,
    ShaderManager* shaderManager,
    PipelineManager* pipelineManager,
    BindGroupManager* bindGroupManager,
    std::span<const PassTargetState> passTargetStates,
    RenderQueue& outRenderQueue) {
    auto view = scene.registry.view<const StaticMeshComponent, const WorldMatrixComponent, const MaterialInstanceComponent>();
    for (auto entity : view) {
        const auto& meshComp = view.get<const StaticMeshComponent>(entity);
        const auto& transformComp = view.get<const WorldMatrixComponent>(entity);
        const auto& matComp = view.get<const MaterialInstanceComponent>(entity);
        
        uint32_t instanceIndex = static_cast<uint32_t>(outRenderQueue.staticInstances.size());
        
        StaticInstanceData instanceData{};
        const glm::mat4& m = transformComp.transform;
        instanceData.row0 = glm::vec4(m[0][0], m[1][0], m[2][0], m[3][0]);
        instanceData.row1 = glm::vec4(m[0][1], m[1][1], m[2][1], m[3][1]);
        instanceData.row2 = glm::vec4(m[0][2], m[1][2], m[2][2], m[3][2]);
        instanceData.customDataIndex = matComp.customDataIndex;
        outRenderQueue.staticInstances.push_back(instanceData);

        AssetView<Mesh> mesh = assetManager->GetMesh(meshComp.meshHandle);
        const MeshAssetFormat::SubMeshInfo& subMesh =
            mesh->GetSubMeshInfo(meshComp.subMeshIndex);
        std::span<const MeshAssetFormat::BufferRange> bufferRanges =
            mesh->GetBufferRanges(subMesh.bufferRangeStart, subMesh.bufferRangeCount);
        AssetView<Material> material = assetManager->GetMaterial(meshComp.materialHandle);

        for (uint32_t passId : passes) {
            Handle shaderHandle =
                shaderManager->GetShaderHandle(passId, material->GetActiveTechniqueID());
            if (!shaderHandle.IsValid()) {
                continue;
            }
            AssetView<ShaderAsset> shader = shaderManager->GetShaderAsset(shaderHandle);

            Handle pipelineHandle =
                pipelineManager->GetOrCreatePipeline(PipelineManager::PipelineConfig{
                    .shader = shader,
                    .layoutId = mesh->GetGlobalVertexStateID(meshComp.subMeshIndex),
                    .blendMode = BlendMode::Opaque,
                    .depthStencilId = DepthStencilStateManager::kDefaultDepthStateID,
                    .passId = passId,
                    .cullMode = wgpu::CullMode::Back,
                    .targetState = &passTargetStates[passId],
                });

            RenderIntent intent;
            intent.baseInstance = instanceIndex;
            intent.instanceCount = 1;
            intent.pipeline = pipelineManager->GetPipeline(pipelineHandle);
            intent.vertexBuffer = mesh->vertexBuffer;
            intent.indexBuffer = mesh->indexBuffer;
            intent.bufferRange = bufferRanges;
            intent.subMeshInfo = subMesh;
            intent.sortKey = RenderIntent::CreateOpaqueKey(0 /* topology */, pipelineHandle.index,
                                                           meshComp.materialHandle.index,
                                                           meshComp.meshHandle.index, 0);

            intent.bindGroup = bindGroupManager->GetBindGroup(material.handle);

            outRenderQueue.renderIntents[passId].push_back(intent);
        }
    }

    outRenderQueue.cameraData = scene.cameraData;
}

namespace core::render {

SceneRenderer::SceneRenderer(Device* device,
                             AssetManager* assetManager,
                             wgpu::BindGroupLayoutDescriptor globalBindGroupLayoutDesc)
    : m_device(device),
      m_assetManager(assetManager),
      m_layoutCache(std::make_unique<LayoutCache>(device)),
      m_vertexLayoutManager(std::make_unique<VertexLayoutManager>()),
      m_passManager(std::make_unique<PassManager>()),
      m_textureManager(std::make_unique<TextureManager>(device, assetManager)),
      m_materialManager(
          std::make_unique<MaterialManager>(device, assetManager, m_textureManager.get())),
      m_meshManager(
          std::make_unique<MeshManager>(device, assetManager, m_vertexLayoutManager.get())),
      m_pipelineManager(std::make_unique<PipelineManager>(device,
                                                          m_layoutCache.get(),
                                                          m_passManager.get(),
                                                          m_vertexLayoutManager.get(),
                                                          globalBindGroupLayoutDesc)),
      m_shaderManager(std::make_unique<ShaderManager>(device,
                                                      assetManager,
                                                      m_layoutCache.get(),
                                                      m_passManager.get(),
                                                      m_materialManager.get())),
      m_bindGroupManager(std::make_unique<BindGroupManager>(device,
                                                            m_shaderManager.get(),
                                                            m_materialManager.get())),
      m_renderGraph(device) {
    m_passManager->RegisterPass<pass::ForwardRenderPass>("ForwardRenderPass");
    m_passManager->RegisterPass<pass::DeferredGBufferPass>("DeferredGBufferPass");
    m_passManager->RegisterPass<pass::DeferredLightingPass>("DeferredLightingPass");

    CameraUniformData cameraUniformData;
    wgpu::Buffer globalUniform = device->CreateBufferFromData(
        &cameraUniformData, sizeof(CameraUniformData), wgpu::BufferUsage::Uniform);
    m_globalUniformBuffer = std::move(globalUniform);

    wgpu::SamplerDescriptor desc{
        .addressModeU = wgpu::AddressMode::Repeat,
        .addressModeV = wgpu::AddressMode::Repeat,
        .addressModeW = wgpu::AddressMode::Repeat,
        .magFilter = wgpu::FilterMode::Linear,
        .minFilter = wgpu::FilterMode::Linear,
        .mipmapFilter = wgpu::MipmapFilterMode::Linear,
    };
    wgpu::Sampler linearRepeat = device->GetDevice().CreateSampler(&desc);
    m_linearRepeatSampler = std::move(linearRepeat);

    wgpu::SamplerDescriptor pointSamplerDesc{
        .addressModeU = wgpu::AddressMode::ClampToEdge,
        .addressModeV = wgpu::AddressMode::ClampToEdge,
        .addressModeW = wgpu::AddressMode::ClampToEdge,
        .magFilter = wgpu::FilterMode::Nearest,
        .minFilter = wgpu::FilterMode::Nearest,
        .mipmapFilter = wgpu::MipmapFilterMode::Nearest,
    };
    wgpu::Sampler pointSampler = device->GetDevice().CreateSampler(&pointSamplerDesc);
    m_pointSampler = std::move(pointSampler);

    std::array<wgpu::BindGroupEntry, 3> bindGroupEntries{wgpu::BindGroupEntry{
                                                             .binding = 0,
                                                             .buffer = m_globalUniformBuffer,
                                                             .offset = 0,
                                                             .size = sizeof(CameraUniformData),
                                                         },
                                                         wgpu::BindGroupEntry{
                                                             .binding = 1,
                                                             .sampler = m_linearRepeatSampler,
                                                         },
                                                         wgpu::BindGroupEntry{
                                                             .binding = 2,
                                                             .sampler = m_pointSampler,
                                                         }

    };
    m_globalBindGroup = device->CreateBindGroup(wgpu::BindGroupDescriptor{
        .layout = m_layoutCache->GetBindGroupLayout(globalBindGroupLayoutDesc),
        .entryCount = bindGroupEntries.size(),
        .entries = bindGroupEntries.data(),
    });

    m_depthTexture = device->CreateTexture(wgpu::TextureDescriptor{
        .usage = wgpu::TextureUsage::RenderAttachment,
        .size =
            wgpu::Extent3D{
                .width = device->GetSurfaceConfig().width,
                .height = device->GetSurfaceConfig().height,
                .depthOrArrayLayers = 1,
            },
        .format = wgpu::TextureFormat::Depth24Plus,
    });
}

void SceneRenderer::Setup(std::span<uint32_t> passIDs) {
    DependencyGraph dependencyGraph = m_renderGraph.Compile(passIDs, m_passManager.get());
    m_compiledGraph = m_renderGraph.Compile(std::move(dependencyGraph), m_passManager.get(),
                                            m_shaderManager.get());
}

inline void RadixSortRenderIntents64(std::vector<core::render::RenderIntent>& intents) {
    if (intents.empty()) {
        return;
    }

    std::vector<core::render::RenderIntent> scratchBuffer(intents.size());

    auto* src = &intents;
    auto* dst = &scratchBuffer;

    const int bitsPerPass = 8;
    const int bucketCount = 1 << bitsPerPass;
    const int passCount = sizeof(uint64_t);

    for (int pass = 0; pass < passCount; ++pass) {
        size_t counts[bucketCount] = {0};
        size_t offsets[bucketCount] = {0};

        const int shift = pass * bitsPerPass;

        for (const auto& intent : *src) {
            uint8_t bucket = (intent.sortKey >> shift) & 0xFF;
            counts[bucket]++;
        }

        size_t total = 0;
        for (int i = 0; i < bucketCount; ++i) {
            offsets[i] = total;
            total += counts[i];
        }

        for (const auto& intent : *src) {
            uint8_t bucket = (intent.sortKey >> shift) & 0xFF;
            (*dst)[offsets[bucket]++] = intent;
        }

        std::swap(src, dst);
    }

    if (src != &intents) {
        intents = std::move(scratchBuffer);
    }
}

void SceneRenderer::Render(const Scene& scene, std::span<uint32_t> passIDs) {
    m_renderQueue.Clear();

    std::span<Handle> dirties = m_materialManager->GetDirtyMaterials();
    for (auto handle : dirties) {
        m_bindGroupManager->UpdateBindGroup(handle);
    }
    m_materialManager->ClearDirties();

    SceneCuller::ExtractRenderQueue(scene, passIDs, m_assetManager, m_shaderManager.get(),
                                    m_pipelineManager.get(), m_bindGroupManager.get(),
                                    m_compiledGraph.targetStates, m_renderQueue);
    Prepare(m_compiledGraph, m_renderQueue);
    Execute(m_compiledGraph, m_renderQueue);
}

void SceneRenderer::Prepare(CompiledGraph& compiledGraph, RenderQueue& renderQueue) {
    for (uint32_t i = 0; i < compiledGraph.executionOrder.size(); ++i) {
        uint32_t nodeId = compiledGraph.executionOrder[i];

        if (renderQueue.renderIntents[nodeId].empty()) {
            Handle shaderHandle =
                m_shaderManager->GetShaderHandle(nodeId, MaterialManager::kEmptyMaterialTechniqe);
            if (renderQueue.renderIntents[nodeId].empty() && shaderHandle.IsValid()) {
                PipelineManager::PipelineConfig config{};
                config.shader = m_shaderManager->GetShaderAsset(shaderHandle);
                config.layoutId = VertexLayoutManager::kVoidVertexLayout;
                config.passId = nodeId;
                config.depthStencilId = DepthStencilStateManager::kNullStateID;
                config.targetState = &compiledGraph.targetStates[nodeId];

                Handle pipelineHandle = m_pipelineManager->GetOrCreatePipeline(config);

                renderQueue.proceduralPipelines[nodeId] =
                    m_pipelineManager->GetPipeline(pipelineHandle);
            }
        }

        RadixSortRenderIntents64(renderQueue.renderIntents[nodeId]);
    }
}

void SceneRenderer::Execute(CompiledGraph& compiledGraph, RenderQueue& renderQueue) {
    auto d = m_device->GetDevice();
    compiledGraph.SetSurfaceTexture(m_device->GetCurrentTexture());

    auto& cameraData = renderQueue.cameraData;
    d.GetQueue().WriteBuffer(m_globalUniformBuffer, 0, &cameraData, sizeof(CameraUniformData));

    auto commandEncoder = d.CreateCommandEncoder();
    for (uint32_t i = 0; i < compiledGraph.executionOrder.size(); ++i) {
        uint32_t nodeId = compiledGraph.executionOrder[i];

        const RenderNode& node = compiledGraph.renderNodes[nodeId];

        wgpu::RenderPassDescriptor renderPassDescriptor{};

        std::vector<wgpu::RenderPassColorAttachment> colorAttachments;
        colorAttachments.reserve(node.attachments.size());
        for (const auto& attach : node.attachments) {
            wgpu::TextureView view = attach.resource.CreateView();
            colorAttachments.push_back(wgpu::RenderPassColorAttachment{
                .view = view,
                .loadOp = attach.colorAttach.loadOp,
                .storeOp = attach.colorAttach.storeOp,
                .clearValue = attach.colorAttach.clearValue,
            });
        }

        renderPassDescriptor.colorAttachmentCount = colorAttachments.size();
        renderPassDescriptor.colorAttachments = colorAttachments.data();

        wgpu::RenderPassDepthStencilAttachment depthStencilAttach;
        if (node.depthStencilAttachment) {
            const auto& desc = node.depthStencilAttachment->depthStencilAttach;

            depthStencilAttach.view = node.depthStencilAttachment->resource.CreateView();
            depthStencilAttach.depthClearValue = desc.depthClearValue;
            depthStencilAttach.depthLoadOp = desc.depthLoadOp;
            depthStencilAttach.depthStoreOp = desc.depthStoreOp;
            depthStencilAttach.stencilClearValue = desc.stencilClearValue;
            depthStencilAttach.stencilLoadOp = desc.stencilLoadOp;
            depthStencilAttach.stencilStoreOp = desc.stencilStoreOp;

            renderPassDescriptor.depthStencilAttachment = &depthStencilAttach;
        }
        wgpu::RenderPassEncoder encoder = commandEncoder.BeginRenderPass(&renderPassDescriptor);
        encoder.SetBindGroup(0, m_globalBindGroup);
        encoder.SetBindGroup(BindSlot::Pass, node.m_bindGroup);
        node.pass->Execute(encoder,
                           {
                               .intents = renderQueue.renderIntents[nodeId],
                               .assetRegistry = m_assetManager->GetRegistry(),
                               .proceduralPipeline = renderQueue.proceduralPipelines[nodeId],
                           });
        encoder.End();
    }

    auto commandBuffer = commandEncoder.Finish();
    d.GetQueue().Submit(1, &commandBuffer);
}

}  // namespace core::render
