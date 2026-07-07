#pragma once

#include <webgpu/webgpu_cpp.h>
#include <array>

#include "AssetManager.h"
#include "IRenderPass.h"
#include "render/SceneRenderer.h"
#include "render/backend/PipelineManager.h"
#include "render/render.h"
#include "render/resource/Material.h"

namespace core::render {

struct RenderNode {

    IRenderPass* pass = nullptr;
    struct ColorAttach {
        uint32_t resourceIdx;
        ColorAttachmentInfo colorAttach;
    };
    struct DepthStencilAttach {
        uint32_t resourceIdx;
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
    uint32_t colorAttachementsVirTextureIndex;
    core::PropertyId colorAttachResourcePropertyID;
    core::render::ColorAttachmentInfo colorAttachment;
};

struct VirtualDepthDtencilAttach {
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
    std::optional<VirtualDepthDtencilAttach> depthStencil;

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

class RenderGraph {
  public:
    RenderGraph(Device* device,
                AssetManager* assetManager,
                ShaderManager* shaderManager,
                PipelineManager* pipelineManager,
                const wgpu::BindGroupLayout globalBindGroupLayout);

    RenderGraph(RenderGraph&& rhs) noexcept = default;

    void Setup(std::span<uint32_t> passes, PassManager* passManager);

    void Prepare(RenderQueue& renderQueue, PipelineManager* pipelineManager);
    void Execute(RenderQueue& frameContext);

    std::span<const PassTargetState> GetTargetStates();

  private:
    Device* m_device;

    AssetManager* m_assetManager;  // TODO!(#2, #8; Sunghyun) m_assetManager will be removed in
    // future, and each RenderPass will directly access asset manager
    // to get necessary resource. But for now, we put asset manager
    // in RenderGraph for simplicity.
    ShaderManager* m_shaderManager;
    PipelineManager* m_pipelineManager;

    wgpu::BindGroup m_globalBindGroup;
    wgpu::Buffer m_globalUniformBuffer;
    wgpu::Sampler m_linearRepeatSampler;
    wgpu::Sampler m_pointSampler;
    wgpu::Texture m_depthTexture;

    std::array<RenderNode, PassManager::kMaxPasses> m_renderNodes{};
    std::array<PassTargetState, PassManager::kMaxPasses> m_targetStates{};
    std::vector<uint32_t> m_executionOrder;

    TransientResourcePool m_vra;
};
}  // namespace core::render
