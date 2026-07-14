#include <array>
#include <ranges>

#include "RenderGraph.h"
#include "render/resource/Material.h"
#include "render/resource/ShaderManager.h"

core::render::TransientResourcePool::TransientResourcePool(Device* device) : m_device(device) {
    m_textures.resize(1);  // Reserve index 0 for scene frame texture
    m_config = device->GetSurfaceConfig();
}

void core::render::TransientResourcePool::InjectExternalResource(uint32_t handle,
                                                                 wgpu::Texture externalTexture) {
    if (handle >= m_textures.size()) {
        m_textures.resize(handle + 1);
    }
    m_textures[handle] = externalTexture;
}

uint32_t core::render::TransientResourcePool::Attache(const TextureDescriptor& desc) {
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
                if (const auto* relSize = std::get_if<RelativeSize>(&desc.size)) {
                    return wgpu::Extent3D{
                        .width = static_cast<uint32_t>(m_config.width * relSize->widthRatio),
                        .height = static_cast<uint32_t>(m_config.height * relSize->heightRatio),
                    };
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

void core::render::TransientResourcePool::Release(const TextureDescriptor& desc,
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

core::render::RenderGraph::RenderGraph(Device* device) : m_device(device) {}

core::render::CompiledGraph core::render::RenderGraph::Compile(std::span<uint32_t> passes,
                                                               PassManager* passManager,
                                                               ShaderManager* shaderManager,
                                                               TransientResourcePool& vra) {
    CompiledGraph compiledGraph;
    std::array<PassSetupContext, PassManager::kMaxPasses> setupContexts;
    std::array<VirtualPassNode, PassManager::kMaxPasses> virtualPasses;
    std::unordered_map<PropertyId, uint32_t> subResourceMap;
    std::vector<SubResource> subResources;

    subResources.push_back(SubResource{
        .textureDesc =
            TextureDescriptor{
                .label = "SceneColor",
                .usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::TextureBinding,
                .dimension = wgpu::TextureDimension::e2D,
                .size = RelativeSize{1.0f, 1.0f},
                .format = m_device->GetSurfaceConfig().format,
            },
        .actualResource = TransientResourcePool::kSurfaceTextureIndex});
    subResourceMap[ToPropertyID(PassSetupContext::kSceneColorName)] =
        PassSetupContext::kSceneColorHandle.index;

    for (uint32_t passId : passes) {
        IRenderPass* pass_ptr = passManager->GetPass(passId);
        pass_ptr->Setup(setupContexts[passId]);

        const PassSetupContext& ctx = setupContexts[passId];

        // start at [1] because we need to skip the scene color texture
        for (uint32_t i = PassSetupContext::kSceneColorHandle.index + 1;
             i < ctx.m_declaredTextures.size(); ++i) {
            const auto& locTexture = ctx.m_declaredTextures[i];
            if (subResourceMap.contains(ToPropertyID(locTexture.name))) {
                assert(false && "Pass declared a texture twice!");
                continue;
            }
            subResourceMap[ToPropertyID(locTexture.name)] = subResources.size();
            subResources.push_back(SubResource{.textureDesc = locTexture.textureDesc});
        }
    }

    for (uint32_t passId : passes) {
        const PassSetupContext& ctx = setupContexts[passId];

        auto GetGlobalResource = [&](const std::string& name) -> uint32_t {
            auto it = subResourceMap.find(ToPropertyID(name));
            assert(it != subResourceMap.end() && "Pass required an undeclared texture!");
            return it->second;
        };

        for (auto& colorAttach : ctx.m_colorAttachments) {
            LocalTexture locTex = ctx.m_requiredTextures[colorAttach.virTextureHandle.index];
            uint32_t virRsrcIndex = GetGlobalResource(locTex.name);
            subResources[virRsrcIndex].writePassInfos.push_back({passId});
            virtualPasses[passId].color.push_back(
                {.colorAttachementsVirTextureIndex = virRsrcIndex,
                 .colorAttachResourcePropertyID = ToPropertyID(locTex.name),
                 .colorAttachment = colorAttach.attachemntInfo});
            virtualPasses[passId].targetState.colorTargetFormats.push_back(
                subResources[virRsrcIndex].textureDesc.format);
        }

        if (ctx.m_depthStencilAttachment.has_value()) {
            LocalTexture locTex =
                ctx.m_requiredTextures[ctx.m_depthStencilAttachment->virTextureHandle.index];
            uint32_t virRsrcIndex = GetGlobalResource(locTex.name);
            subResources[virRsrcIndex].writePassInfos.push_back({passId});
            virtualPasses[passId].depthStencil = VirtualDepthDtencilAttach{
                .virTextureIndex = virRsrcIndex,
                .depthStencilResourcePropertyID = ToPropertyID(locTex.name),
                .depthStencilAttachment = ctx.m_depthStencilAttachment->attachmentInfo};
            virtualPasses[passId].targetState.depthStencilFormat =
                subResources[virRsrcIndex].textureDesc.format;
        }

        for (auto& read : ctx.m_readTextures) {
            LocalTexture locTex = ctx.m_requiredTextures[read.virTexture.index];
            uint32_t virRsrcIndex = GetGlobalResource(locTex.name);
            subResources[virRsrcIndex].readPassInfos.push_back({passId});
            virtualPasses[passId].readInfos.push_back(
                {.virTextureIndex = virRsrcIndex,
                 .bindingResourcePropertyId = ToPropertyID(locTex.name),
                 .viewDesc = read.viewDesc});
        }
    }

    for (uint32_t i = 0; i < subResources.size(); ++i) {
        const auto resource = subResources[i];
        if (resource.writePassInfos.size() > 1) {
            assert(
                false &&
                "Currently we only support one writer for the scene color. Multiple writers will "
                "cause conflict. We will support multiple writers in the future.");
        }
        if (resource.writePassInfos.empty())
        {
            // TODO!(Sunghyun) return unexpected behavior here.
            // We should return an error or unexpected behavior instead of silently ignoring it.
            assert(false && "No pass writes to this resource!");
            continue;
        }
        uint32_t writePassId = resource.writePassInfos[0].passId;
        VirtualPassNode& writePassNode = virtualPasses[writePassId];

        for (auto readPassInfo : resource.readPassInfos) {
            writePassNode.successorNodes.push_back(readPassInfo.passId);
            virtualPasses[readPassInfo.passId].predecessorNodes.push_back(writePassId);
        }
    }

    const auto& sceneColorWriters = subResources[PassSetupContext::kSceneColorHandle.index].writePassInfos;
    assert(!sceneColorWriters.empty() && "No pass writes to SceneColor!");
    uint32_t sceneColorPassId = sceneColorWriters.empty() ? 0 : sceneColorWriters[0].passId;

    std::array<bool, 255> visited{};
    std::array<bool, 255> onStack{};
    std::vector<uint32_t> executionOrder;

    std::function<void(uint32_t)> dfs = [&](uint32_t nodeId) {
        visited[nodeId] = true;
        onStack[nodeId] = true;
        for (auto predecessor : virtualPasses[nodeId].predecessorNodes) {
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
    compiledGraph.executionOrder = std::move(executionOrder);

    struct ResourceUsageInfo {
        uint32_t firstUse = UINT32_MAX;
        uint32_t lastUse = 0;
    };

    std::vector<ResourceUsageInfo> resourceUsageInfo(subResources.size());
    for (uint32_t i = 0; i < compiledGraph.executionOrder.size(); ++i) {
        uint32_t nodeId = compiledGraph.executionOrder[i];
        for (const auto& colorAttach : virtualPasses[nodeId].color) {
            uint32_t virRsrcIndex = colorAttach.colorAttachementsVirTextureIndex;
            resourceUsageInfo[virRsrcIndex].firstUse =
                std::min(resourceUsageInfo[virRsrcIndex].firstUse, i);
            resourceUsageInfo[virRsrcIndex].lastUse =
                std::max(resourceUsageInfo[virRsrcIndex].lastUse, i);
        }
        if (virtualPasses[nodeId].depthStencil.has_value()) {
            const auto& resourceIdx = virtualPasses[nodeId].depthStencil->virTextureIndex;
            resourceUsageInfo[resourceIdx].firstUse =
                std::min(resourceUsageInfo[resourceIdx].firstUse, i);
            resourceUsageInfo[resourceIdx].lastUse =
                std::max(resourceUsageInfo[resourceIdx].lastUse, i);
        }

        for (auto readInfo : virtualPasses[nodeId].readInfos) {
            resourceUsageInfo[readInfo.virTextureIndex].firstUse =
                std::min(resourceUsageInfo[readInfo.virTextureIndex].firstUse, i);
            resourceUsageInfo[readInfo.virTextureIndex].lastUse =
                std::max(resourceUsageInfo[readInfo.virTextureIndex].lastUse, i);
        }
    }

    for (uint32_t step = 0; step < compiledGraph.executionOrder.size(); ++step) {
        for (uint32_t resrcIdx = 0; resrcIdx < resourceUsageInfo.size(); ++resrcIdx) {
            const auto& sub = subResources[resrcIdx];
            if (resourceUsageInfo[resrcIdx].firstUse != UINT32_MAX &&
                resourceUsageInfo[resrcIdx].lastUse == step - 1) {
                vra.Release(sub.textureDesc, sub.actualResource);
            }

            if (resourceUsageInfo[resrcIdx].firstUse == step &&
                resrcIdx != PassSetupContext::kSceneColorHandle.index) {
                TransientResourcePool::Handle actual = vra.Attache(sub.textureDesc);
                subResources[resrcIdx].actualResource = actual;
            }
        }
    }

    for (uint32_t step = 0; step < compiledGraph.executionOrder.size(); ++step) {
        uint32_t nodeIdx = compiledGraph.executionOrder[step];

        wgpu::BindGroup passBindGroup = nullptr;
        std::optional<wgpu::BindGroupLayout> layout =
            shaderManager->GetPassBindGroupLayout(nodeIdx);
        if (layout.has_value()) {
            std::span<const ShaderAssetFormat::Binding> bindingInfo =
                shaderManager->GetPassBindGroupInfo(nodeIdx);
            ResourceResolver resolver(virtualPasses[nodeIdx], subResourceMap, subResources, &vra);
            passBindGroup = BindGroupFactory::Create(m_device, layout.value(), bindingInfo,
                                                     RenderGraphProvider{&resolver});
        }

        std::vector<RenderNode::ColorAttach> colorAttachments =
            virtualPasses[nodeIdx].color |
            std::views::transform([&subResources](const VirtualColorAttach& color) {
                return RenderNode::ColorAttach{
                    .resourceIdx =
                        subResources[color.colorAttachementsVirTextureIndex].actualResource,
                    .colorAttach = color.colorAttachment,
                };
            }) |
            std::ranges::to<std::vector>();
        std::optional<RenderNode::DepthStencilAttach> depthAttach =
            virtualPasses[nodeIdx].depthStencil.transform(
                [&subResources](VirtualDepthDtencilAttach& depth) {
                    return RenderNode::DepthStencilAttach{
                        .resourceIdx = subResources[depth.virTextureIndex].actualResource,
                        .depthStencilAttach = depth.depthStencilAttachment,
                    };
                });
        RenderNode node{
            .pass = passManager->GetPass(nodeIdx),
            .attachments = std::move(colorAttachments),
            .depthStencilAttachment = depthAttach,
            .m_bindGroup = passBindGroup,
        };
        compiledGraph.renderNodes[nodeIdx] = node;
        compiledGraph.targetStates[nodeIdx] = virtualPasses[nodeIdx].targetState;
    }

    compiledGraph.subResources = std::move(subResources);
    compiledGraph.subResourceMap = std::move(subResourceMap);

    return compiledGraph;
}

core::render::ResourceResolver::ResourceResolver(
    const VirtualPassNode& passNode,
    const std::unordered_map<PropertyId, uint32_t>& subResourceMap,
    std::span<const SubResource> subResource,
    TransientResourcePool* resourcePool)
    : m_passNode(passNode),
      m_subResourceMap(subResourceMap),
      m_subResources(subResource),
      m_resourcePool(resourcePool) {}

wgpu::TextureView core::render::ResourceResolver::GetTextureView(PropertyId id) const {
    uint32_t virRsourceIndex = m_subResourceMap.at(id);
    const SubResource& subResource = m_subResources[virRsourceIndex];

    wgpu::Texture texture = m_resourcePool->Get(subResource.actualResource);

    auto it =
        std::ranges::find(m_passNode.readInfos, id, &VirtualReadInfo::bindingResourcePropertyId);
    if (it == m_passNode.readInfos.end()) {
        assert(false);
    }

    const auto optViewDesc = it->viewDesc;
    const auto* viewPtr = optViewDesc.has_value() ? &(*optViewDesc) : nullptr;
    return texture.CreateView(viewPtr);
}

wgpu::TextureView core::render::RenderGraphProvider::GetTextureView(PropertyId id) {
    return resolver->GetTextureView(id);
}
