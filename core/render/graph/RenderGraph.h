#pragma once

#include <webgpu/webgpu_cpp.h>
#include <array>
#include <span>
#include <vector>

#include "IRenderPass.h"
#include "render/backend/PipelineManager.h"
#include "render/render.h"
#include "render/resource/Material.h"

namespace core::render {

struct RenderQueue {
    std::array<std::vector<RenderIntent>, PassManager::kMaxPasses> renderIntents;
    std::array<wgpu::RenderPipeline, PassManager::kMaxPasses> proceduralPipelines;
    std::vector<StaticInstanceData> staticInstances;
    std::vector<SkinnedInstanceData> skinnedInstances;
    CameraUniformData cameraData;

    void Clear() {
        for (uint32_t i = 0; i < PassManager::kMaxPasses; ++i) {
            renderIntents[i].clear();
        }
        staticInstances.clear();
        skinnedInstances.clear();
    }
};

struct RenderNode {
    IRenderPass* pass = nullptr;
    struct ColorAttach {
        wgpu::Texture resource;
        ColorAttachmentInfo colorAttach;
    };
    struct DepthStencilAttach {
        wgpu::Texture resource;
        DepthStencilAttachmentInfo depthStencilAttach;
    };
    std::vector<ColorAttach> attachments;
    std::optional<DepthStencilAttach> depthStencilAttachment = std::nullopt;
    wgpu::BindGroup m_bindGroup = nullptr;
};

class TransientResourcePool {
  public:
    TransientResourcePool(Device* device);
    using Handle = uint32_t;
    TransientResourcePool::Handle Attache(const TextureDescriptor& desc);
    void Release(const TextureDescriptor& desc, TransientResourcePool::Handle handle);

    static constexpr Handle kSurfaceTextureIndex = 0;

    wgpu::Texture Get(uint32_t);
    void InjectExternalResource(uint32_t handle, wgpu::Texture externalTexture);

  private:
    Device* m_device;
    wgpu::SurfaceConfiguration m_config;
    std::vector<wgpu::Texture> m_textures;
    std::multimap<TextureDescriptor, uint32_t> m_activeResources;
    std::multimap<TextureDescriptor, uint32_t> m_freeResources;
};

struct VirtualColorAttach {
    uint32_t colorAttachmentsVirTextureIndex;
    core::PropertyId colorAttachResourcePropertyID;
    core::render::ColorAttachmentInfo colorAttachment;
};

struct VirtualDepthStencilAttach {
    uint32_t virTextureIndex;
    core::PropertyId depthStencilResourcePropertyID;
    core::render::DepthStencilAttachmentInfo depthStencilAttachment;
};

struct VirtualReadInfo {
    uint32_t virTextureIndex;
    core::PropertyId bindingResourcePropertyId;
    std::optional<wgpu::TextureViewDescriptor> viewDesc;
};

struct VirtualPassNode {
    std::vector<VirtualColorAttach> color;
    std::optional<VirtualDepthStencilAttach> depthStencil;

    std::vector<VirtualReadInfo> readInfos;

    std::vector<uint32_t> successorNodes;
    std::vector<uint32_t> predecessorNodes;

    PassTargetState targetState;
};

class ResourceResolver {
  private:
    const VirtualPassNode& m_passNode;
    const std::unordered_map<PropertyId, uint32_t>& m_subResourceMap;
    std::span<const SubResource> m_subResources;
    TransientResourcePool* m_resourcePool;

  public:
    ResourceResolver(const VirtualPassNode& passId,
                     const std::unordered_map<PropertyId, uint32_t>& blackBourd,
                     std::span<const SubResource> subResource,
                     TransientResourcePool* resourcePool);
    wgpu::TextureView GetTextureView(PropertyId id) const;
};

struct RenderGraphProvider {
    const ResourceResolver* resolver;
    wgpu::TextureView GetTextureView(PropertyId id);
};

static_assert(BindGroupResourceProvider<RenderGraphProvider>);

struct ResourceUsageInfo {
    int32_t firstUse = INT32_MAX;
    int32_t lastUse = INT32_MIN;
};

struct DependencyGraph {
    std::vector<uint32_t> executionOrder;
    std::array<PassSetupContext, PassManager::kMaxPasses> setupContexts;
    std::array<VirtualPassNode, PassManager::kMaxPasses> virtualPasses;
    std::unordered_map<PropertyId, uint32_t> subResourceMap;
    std::vector<SubResource> subResources;
    std::vector<ResourceUsageInfo> resourceUsageInfo;
};

struct CompiledGraph {
    std::vector<uint32_t> executionOrder;
    std::array<RenderNode, PassManager::kMaxPasses> renderNodes{};
    std::array<PassTargetState, PassManager::kMaxPasses> targetStates{};
    std::optional<std::pair<uint32_t, uint32_t>> surfaceTargetLocation = std::nullopt;

    void SetSurfaceTexture(wgpu::Texture surface) {
        if (surfaceTargetLocation.has_value()) {
            auto [nodeId, attachIdx] = *surfaceTargetLocation;
            renderNodes[nodeId].attachments[attachIdx].resource = surface;
        }
    }
};

class RenderGraph {
  public:
    RenderGraph(Device* device);

    RenderGraph(RenderGraph&& rhs) noexcept = default;
    RenderGraph& operator=(RenderGraph&& rhs) noexcept = default;

    DependencyGraph Compile(std::span<uint32_t> passes, PassManager* passManager);

    CompiledGraph Compile(DependencyGraph&& dependencyGraph,
                          PassManager* passManager,
                          ShaderManager* shaderManager);

  private:
    Device* m_device;
    TransientResourcePool m_transientResourcePool;
};
}  // namespace core::render
