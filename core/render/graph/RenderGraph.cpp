#include "RenderGraph.h"
#include <array>
#include "render/resource/Material.h"
#include "render/resource/Mesh.h"
#include "render/SceneRenderer.h"

core::render::TransientResourcePool::TransientResourcePool(Device* device) : m_device(device) {
    m_textures.resize(1);  // Reserve index 0 for scene frame texture
}

void core::render::TransientResourcePool::InjectExternalResource(uint32_t handle,
                                                                 wgpu::Texture externalTexture) {
    if (handle >= m_textures.size()) {
        m_textures.resize(handle + 1);
    }
    m_textures[handle] = externalTexture;
}

uint32_t core::render::TransientResourcePool::Attache(
    const PassSetupContext::TextureDescriptor& desc) {
    auto it = m_freeResources.find(desc);
    if (it != m_freeResources.end()) {
        auto nh = m_freeResources.extract(it);
        uint32_t index = nh.mapped();
        m_activeResources.insert(std::move(nh));
        return index;
    }

    wgpu::TextureDescriptor wgpuDescription{
        .usage = desc.usage,
        .dimension = desc.dimension,
        .size =
            [&]() {
                if (const auto* relSize = std::get_if<PassSetupContext::RelativeSize>(&desc.size)) {
                    return wgpu::Extent3D{};
                } else {
                    return *std::get_if<wgpu::Extent3D>(&desc.size);
                }
            }(),
        .format = desc.format,
        .mipLevelCount = desc.mipLevelCount,
        .sampleCount = desc.sampleCount,
        .viewFormatCount = desc.viewFormatCount,
        .viewFormats = desc.viewFormats,
    };

    wgpu::Texture texture = m_device->CreateTexture(wgpuDescription);
    m_textures.push_back(texture);

    uint32_t index = m_textures.size() - 1;
    m_activeResources.insert(std::make_pair(desc, index));
    return index;
}

void core::render::TransientResourcePool::Release(const PassSetupContext::TextureDescriptor& desc,
                                                  TransientResourcePool::Handle handle) {
    auto range = m_activeResources.equal_range(desc);
    for (auto it = range.first; it != range.second; ++it) {
        if (it->second == handle) {
            m_freeResources.insert(m_activeResources.extract(it));
            return;
        }
    }

    assert(false && "Failed to release: Handle not found in active resources!");
}

wgpu::Texture core::render::TransientResourcePool::Get(uint32_t index) {
    return m_textures[index];
}

core::render::RenderGraph::RenderGraph(Device* device,
                                       AssetManager* assetManager,
                                       ShaderManager* shaderManager,
                                       PipelineManager* pipelineManager,
                                       const wgpu::BindGroupLayout globalBindGroupLayout)
    : m_device(device),
      m_assetManager(assetManager),
      m_shaderManager(shaderManager),
      m_pipelineManager(pipelineManager),
      m_vra(device) {
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
    wgpu::Sampler linearRepeat = device->GetDeivce().CreateSampler(&desc);
    m_linearRepeatSampler = std::move(linearRepeat);

    wgpu::SamplerDescriptor pointSamplerDesc{
        .addressModeU = wgpu::AddressMode::ClampToEdge,
        .addressModeV = wgpu::AddressMode::ClampToEdge,
        .addressModeW = wgpu::AddressMode::ClampToEdge,
        .magFilter = wgpu::FilterMode::Nearest,
        .minFilter = wgpu::FilterMode::Nearest,
        .mipmapFilter = wgpu::MipmapFilterMode::Nearest,
    };
    wgpu::Sampler pointSampler = device->GetDeivce().CreateSampler(&pointSamplerDesc);
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
        .layout = globalBindGroupLayout,
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

void core::render::RenderGraph::Setup(std::span<uint32_t> passes, PassManager* passManager) {
    core::render::PassSetupContext context{m_device, m_device->GetSurfaceConfig()};

    std::ranges::for_each(passes, [&](auto& passID) {
        IRenderPass* pass_ptr = passManager->GetPass(passID);
        context.SetPassID(passID);
        pass_ptr->Setup(context);

        m_renderNodes[passID] = RenderNode(std::move(pass_ptr));
    });

    for (uint32_t i = 0; i < context.m_subResources.size(); ++i) {
        const auto resource = context.m_subResources[i];
        if (resource.writePassInfos.size() > 1) {
            assert(
                false &&
                "Currently we only support one writer for the scene color. Multiple writers will "
                "cause conflict. We will support multiple writers in the future.");
        }
        uint32_t writePassId = resource.writePassInfos[0].passId;
        RenderNode& writePassNode = m_renderNodes[writePassId];
        const auto& attach = resource.writePassInfos[0].attach;
        if (const auto* colorPtr = std::get_if<PassSetupContext::ColorAttachment>(&attach)) {
            writePassNode.attachments.push_back(RenderNode::Attach{
                .resourceIdx = i,
                .colorAttach = *colorPtr,
            });
        } else if (const auto* depthPtr =
                       std::get_if<PassSetupContext::DepthStencilAttachment>(&attach)) {
            writePassNode.depthStencilAttachment = RenderNode::DepthStencilAttach{
                .resourceIdx = i,
                .depthStencilAttach = *depthPtr,
            };
        }

        for (auto readPassInfo : resource.readPassIds) {
            writePassNode.successorNodes.push_back(readPassInfo.passId);
            m_renderNodes[readPassInfo.passId].predecessorNodes.push_back(writePassId);
            m_renderNodes[readPassInfo.passId].readResourceIndices.push_back(i);
        }
    }

    uint32_t sceneColorPassId = context.GetSceneColorSubResource().writePassInfos[0].passId;

    std::array<bool, 255> visited{};
    std::array<bool, 255> onStack{};
    std::vector<uint32_t> executionOrder;

    std::function<void(uint32_t)> dfs = [&](uint32_t nodeId) {
        visited[nodeId] = true;
        onStack[nodeId] = true;
        for (auto predecessor : m_renderNodes[nodeId].predecessorNodes) {
            if (!visited[predecessor]) {
                dfs(predecessor);
            } else if (onStack[predecessor]) {
                assert(false && "Cycle detected in render graph!");
            }
        }
        onStack[nodeId] = false;
        executionOrder.push_back(nodeId);
    };

    dfs(sceneColorPassId);
    m_executionOrder = std::move(executionOrder);

    struct ResourceUsageInfo {
        uint32_t firstUse = UINT32_MAX;
        uint32_t lastUse = 0;
    };

    std::vector<ResourceUsageInfo> resourceUsageInfo(context.m_subResources.size());
    for (uint32_t i = 0; i < m_executionOrder.size(); ++i) {
        uint32_t nodeId = m_executionOrder[i];
        for (const auto& attach : m_renderNodes[nodeId].attachments) {
            resourceUsageInfo[attach.resourceIdx].firstUse =
                std::min(resourceUsageInfo[attach.resourceIdx].firstUse, i);
            resourceUsageInfo[attach.resourceIdx].lastUse =
                std::max(resourceUsageInfo[attach.resourceIdx].lastUse, i);
        }
        if (m_renderNodes[nodeId].depthStencilAttachment.has_value()) {
            const auto& attach = m_renderNodes[nodeId].depthStencilAttachment.value();
            resourceUsageInfo[attach.resourceIdx].firstUse =
                std::min(resourceUsageInfo[attach.resourceIdx].firstUse, i);
            resourceUsageInfo[attach.resourceIdx].lastUse =
                std::max(resourceUsageInfo[attach.resourceIdx].lastUse, i);
        }

        for (auto readResourceIdx : m_renderNodes[nodeId].readResourceIndices) {
            resourceUsageInfo[readResourceIdx].firstUse =
                std::min(resourceUsageInfo[readResourceIdx].firstUse, i);
            resourceUsageInfo[readResourceIdx].lastUse =
                std::max(resourceUsageInfo[readResourceIdx].lastUse, i);
        }
    }

    for (uint32_t step = 0; step < m_executionOrder.size(); ++step) {
        for (uint32_t resrcIdx = 0; resrcIdx < resourceUsageInfo.size(); ++resrcIdx) {
            const auto& virResource = context.m_subResources[resrcIdx];
            if (resourceUsageInfo[resrcIdx].lastUse == step - 1) {
                m_vra.Release(virResource.textureDesc, virResource.actualResource);
            }

            if (resourceUsageInfo[resrcIdx].firstUse == step &&
                resrcIdx != PassSetupContext::kSceneColorHandle.index) {
                TransientResourcePool::Handle actual = m_vra.Attache(virResource.textureDesc);
                context.m_subResources[resrcIdx].actualResource = actual;
            }
        }

        uint32_t nodeIdx = m_executionOrder[step];
        for (auto readResourceIdx : m_renderNodes[nodeIdx].readResourceIndices) {
            m_vra.Get(readResourceIdx);
        }

        std::optional<wgpu::BindGroupLayout> layout =
            m_shaderManager->GetPassBindGroupLayout(nodeIdx);
        if (layout.has_value()) {
            std::span<const ShaderAssetFormat::Binding> bindingInfo =
                m_shaderManager->GetPassBindGroupInfo(nodeIdx);
            const BlackBoard& blackBoard = context.GetBlackBoard();
            ResourceResolver resolver(nodeIdx, &blackBoard, context.m_subResources, &m_vra);
            wgpu::BindGroup passBindGroup = BindGroupFactory::Create(
                m_device, layout.value(), bindingInfo, RenderGraphProvider{&resolver});
            m_renderNodes[nodeIdx].m_bindGroup = passBindGroup;
        }
    }
    m_passSetupContext = std::move(context);
}

void core::render::RenderGraph::Prepare(RenderQueue& renderQueue,
                                        PipelineManager* pipelineManager) {
    for (uint32_t i = 0; i < m_executionOrder.size(); ++i) {
        uint32_t nodeId = m_executionOrder[i];

        if (renderQueue.renderIntents[nodeId].empty()) {
            auto* pass = m_renderNodes[nodeId].pass;

            Handle shaderHandle =
                m_shaderManager->GetShaderHandle(nodeId, MaterialManager::kEmptyMaterialTechniqe);
            if (renderQueue.renderIntents[nodeId].empty() && shaderHandle.IsValid()) {
                PipelineManager::PipelineConfig config{};
                config.shader = m_shaderManager->GetShaderAsset(shaderHandle);
                config.layoutId = VertexLayoutManager::kVoidVertexLayout;
                config.passId = nodeId;
                config.depthStencilId = DepthStencilStateManager::kNullStateID;

                // 4. 멀티스레드 진입 전(Execute 이전)에 캐시에서 가져오거나 새로 구움
                Handle pipelineHandle = pipelineManager->GetOrCreatePipeline(config);

                // 5. 구워진 파이프라인을 저장 (더미 Intent를 쓰지 않기로 했으므로)
                // 패스 로직이 Execute 중에 꺼내 쓸 수 있도록 RenderQueue의 별도 공간에 꽂아 넣음
                renderQueue.proceduralPipelines[nodeId] =
                    pipelineManager->GetPipeline(pipelineHandle);
            }
        }
    }
}

void core::render::RenderGraph::Execute(RenderQueue& renderQueue) {
    auto d = m_device->GetDeivce();
    m_vra.InjectExternalResource(0, m_device->GetCurrentTexture());

    auto& cameraData = renderQueue.cameraData;
    d.GetQueue().WriteBuffer(m_globalUniformBuffer, 0, &cameraData, sizeof(CameraUniformData));

    auto commandEncoder = d.CreateCommandEncoder();
    for (uint32_t i = 0; i < m_executionOrder.size(); ++i) {
        uint32_t nodeId = m_executionOrder[i];

        wgpu::RenderPassDescriptor renderPassDescriptor{};

        std::vector<wgpu::RenderPassColorAttachment> colorAttachments;
        colorAttachments.reserve(m_renderNodes[nodeId].attachments.size());
        auto& subResources = m_passSetupContext->m_subResources;
        for (const auto& attach : m_renderNodes[nodeId].attachments) {
            const auto& subResource = subResources[attach.resourceIdx];
            wgpu::TextureView view = m_vra.Get(subResource.actualResource).CreateView();
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
        if (m_renderNodes[nodeId].depthStencilAttachment) {
            const auto& subResource =
                subResources[m_renderNodes[nodeId].depthStencilAttachment->resourceIdx];
            const auto& desc = m_renderNodes[nodeId].depthStencilAttachment->depthStencilAttach;

            depthStencilAttach.view = m_vra.Get(subResource.actualResource).CreateView();
            depthStencilAttach.depthClearValue = desc.depthClearValue;
            depthStencilAttach.depthLoadOp = desc.depthLoadOp;
            depthStencilAttach.depthStoreOp = desc.depthStoreOp;
            depthStencilAttach.stencilClearValue = desc.stencilClearValue;
            depthStencilAttach.stencilLoadOp = desc.stencilLoadOp;
            depthStencilAttach.stencilStoreOp = desc.stencilStoreOp;

            renderPassDescriptor.depthStencilAttachment = &depthStencilAttach;
        }
        auto pass = commandEncoder.BeginRenderPass(&renderPassDescriptor);
        pass.SetBindGroup(0, m_globalBindGroup);
        pass.SetBindGroup(BindSlot::Pass, m_renderNodes[nodeId].m_bindGroup);
        m_renderNodes[nodeId].pass->Execute(
            pass, {
                      .intents = renderQueue.renderIntents[nodeId],
                      .assetRegistry = m_assetManager->GetRegistry(),
                      .proceduralPipeline = renderQueue.proceduralPipelines[nodeId],
                  });
        pass.End();
    }

    auto commandBuffer = commandEncoder.Finish();
    d.GetQueue().Submit(1, &commandBuffer);
}

core::render::ResourceResolver::ResourceResolver(
    uint32_t passId,
    const BlackBoard* blackBourd,
    std::span<const PassSetupContext::SubResource> subResource,
    TransientResourcePool* resourcePool)
    : m_passId(passId),
      m_blackBoard(blackBourd),
      m_subResources(subResource),
      m_resourcePool(resourcePool) {}

wgpu::TextureView core::render::ResourceResolver::GetTextureView(PropertyId id) const {
    Handle virRsourceHandle = m_blackBoard->Get(id);
    const PassSetupContext::SubResource& subResource = m_subResources[virRsourceHandle.index];

    wgpu::Texture texture = m_resourcePool->Get(subResource.actualResource);

    auto it = std::ranges::find(subResource.readPassIds, m_passId,
                                &PassSetupContext::SubResource::ReadInfo::passId);
    if (it == subResource.readPassIds.end()) {
        assert(false);
    }
    const auto optViewDesc = it->viewDesc;
    const auto* viewPtr = optViewDesc.has_value() ? &(*optViewDesc) : nullptr;
    return texture.CreateView(viewPtr);
}

wgpu::TextureView core::render::RenderGraphProvider::GetTextureView(PropertyId id) {
    return resolver->GetTextureView(id);
}
