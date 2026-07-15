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
    std::span<const glm::mat4x4> transforms;
    CameraUniformData cameraData;

    void Clear() {
        for (uint32_t i = 0; i < PassManager::kMaxPasses; ++i) {
            renderIntents[i].clear();
        }
        transforms = {};
    }
};

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

struct CompiledGraph {
    std::vector<uint32_t> executionOrder;
    std::array<RenderNode, PassManager::kMaxPasses> renderNodes{};
    std::array<PassTargetState, PassManager::kMaxPasses> targetStates{};

    std::vector<SubResource> subResources;
    std::unordered_map<PropertyId, uint32_t> subResourceMap;
};

class RenderGraph {
  public:
    RenderGraph(Device* device);

    RenderGraph(RenderGraph&& rhs) noexcept = default;
    RenderGraph& operator=(RenderGraph&& rhs) noexcept = default;

    CompiledGraph Compile(std::span<uint32_t> passes,
                          PassManager* passManager,
                          ShaderManager* shaderManager,
                          TransientResourcePool& vra);

  private:
    Device* m_device;
};
}  // namespace core::render
