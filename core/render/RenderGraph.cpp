#include "RenderGraph.h"
#include <array>
#include "Material.h"
#include "Mesh.h"
#include "SceneRenderer.h"

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
                                       PipelineManager* pipelineManager,
                                       const wgpu::BindGroupLayout globalBindGroupLayout)
    : m_device(device),
      m_assetManager(assetManager),
      m_pipelineManager(pipelineManager),
      m_vra(device) {
    CameraUniformData cameraUniformData;
    wgpu::Buffer globalUniform = device->CreateBufferFromData(
        &cameraUniformData, sizeof(CameraUniformData), wgpu::BufferUsage::Uniform);
    m_globalUniformBuffer = std::move(globalUniform);

    std::array<wgpu::BindGroupEntry, 1> bindGroupEntries{wgpu::BindGroupEntry{
        .binding = 0,
        .buffer = m_globalUniformBuffer,
        .offset = 0,
        .size = sizeof(CameraUniformData),
    }};
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

void core::render::RenderGraph::Setup(std::vector<std::unique_ptr<IRenderPass>>& passes) {
    core::render::PassSetupContext context{m_device, m_device->GetSurfaceConfig()};

    auto pass_view = passes | std::views::enumerate;

    std::ranges::for_each(pass_view, [&](auto&& item) {
        auto [i, pass_ptr] = std::move(item);

        const uint8_t passIdx = static_cast<uint8_t>(i);

        context.SetPassID(passIdx);
        pass_ptr->Setup(context);

        m_renderNodes[passIdx] = RenderNode(std::move(pass_ptr));
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

        for (auto readPassId : resource.readPassIds) {
            writePassNode.successorNodes.push_back(readPassId);
            m_renderNodes[readPassId].predecessorNodes.push_back(writePassId);
            m_renderNodes[readPassId].readResourceIndices.push_back(i);
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
            context.m_subResources[attach.resourceIdx];
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
    }
    m_passSetupContext = std::move(context);
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
        m_renderNodes[nodeId].pass->Execute(pass, renderQueue.renderIntents,
                                            m_assetManager->GetRegistry(),
                                            m_pipelineManager->GetAllPipelines());
        pass.End();
    }

    auto commandBuffer = commandEncoder.Finish();
    d.GetQueue().Submit(1, &commandBuffer);
}
