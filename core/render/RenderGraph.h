#pragma once

#include <webgpu/webgpu_cpp.h>
#include <array>
#include <string>

#include "AssetManager.h"
#include "IRenderPass.h"
#include "Material.h"
#include "PipelineManager.h"
#include "SceneRenderer.h"
#include "render.h"

namespace core::render {

struct RenderNode {
    RenderNode() = default;
    RenderNode(IRenderPass* pass) : pass(std::move(pass)) {}

    IRenderPass* pass = nullptr;
    struct Attach {
        uint32_t resourceIdx;
        PassSetupContext::ColorAttachment colorAttach;
    };
    struct DepthStencilAttach {
        uint32_t resourceIdx;
        PassSetupContext::DepthStencilAttachment depthStencilAttach;
    };
    std::vector<Attach> attachments;
    std::optional<DepthStencilAttach> depthStencilAttachment = std::nullopt;
    std::vector<uint32_t> readResourceIndices;
    wgpu::BindGroup m_bindGroup = nullptr;
    std::vector<uint32_t> predecessorNodes;
    std::vector<uint32_t> successorNodes;
};

class TransientResourcePool {
  public:
    TransientResourcePool(Device* device);
    using Handle = uint32_t;
    TransientResourcePool::Handle Attache(const PassSetupContext::TextureDescriptor& desc);
    void Release(const PassSetupContext::TextureDescriptor& desc,
                 TransientResourcePool::Handle handle);

    wgpu::Texture Get(uint32_t);
    void InjectExternalResource(uint32_t handle, wgpu::Texture externalTexture);

  private:
    Device* m_device;
    std::vector<wgpu::Texture> m_textures;
    std::multimap<PassSetupContext::TextureDescriptor, uint32_t> m_activeResources;
    std::multimap<PassSetupContext::TextureDescriptor, uint32_t> m_freeResources;
};

class ResourceResolver {
  private:
    uint32_t m_passId;
    const BlackBoard* m_blackBoard;
    std::span<const PassSetupContext::SubResource> m_subResources;
    TransientResourcePool* m_resourcePool;

  public:
    ResourceResolver(
        uint32_t passId,
        const BlackBoard* blackBourd,
                     std::span<const PassSetupContext::SubResource> subResource,
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

  private:
    Device* m_device;

    AssetManager* m_assetManager;  // TODO!(#2, #8; Sunghyun) m_assetManager will be removed in
                                   // future, and each RenderPass will directly access asset manager
                                   // to get necessary resource. But for now, we put asset manager
                                   // in RenderGraph for simplicity.
    PipelineManager* m_pipelineManager;
    ShaderManager* m_shaderManager;

    wgpu::BindGroup m_globalBindGroup;
    wgpu::Buffer m_globalUniformBuffer;
    wgpu::Sampler m_linearRepeatSampler;
    wgpu::Sampler m_pointSampler;
    wgpu::Texture m_depthTexture;

    std::array<RenderNode, 255> m_renderNodes{};
    std::array<wgpu::BindGroup, 255> m_passBindGroups{nullptr};
    std::vector<uint32_t> m_executionOrder;

    TransientResourcePool m_vra;
    std::optional<PassSetupContext> m_passSetupContext;
};
}  // namespace core::render