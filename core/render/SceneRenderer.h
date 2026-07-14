#pragma once
#include <memory>
#include <span>
#include "Scene.h"
#include "render.h"
#include "render/backend/BindGroupManager.h"
#include "render/backend/PipelineManager.h"
#include "render/graph/IRenderPass.h"
#include "render/graph/RenderGraph.h"
#include "render/resource/MaterialManager.h"
#include "render/resource/MeshManager.h"
#include "render/resource/ShaderManager.h"
#include "render/resource/TextureManager.h"

namespace core::render {

class SceneCuller {
  public:
    static void ExtractRenderQueue(const Scene& scene,
                                   std::span<uint32_t> passes,
                                   AssetManager* assetManager,
                                   ShaderManager* shaderManager,
                                   PipelineManager* pipelineManager,
                                   BindGroupManager* bindGroupManager,
                                   std::span<const PassTargetState> passTargetStates,
                                   RenderQueue& outRenderQueue);
};

class SceneRenderer {
  public:
    SceneRenderer(Device* device,
                  AssetManager* assetManager,
                  wgpu::BindGroupLayoutDescriptor globalBindGroupLayoutDesc);
    ~SceneRenderer() = default;

    SceneRenderer(const SceneRenderer&) = delete;
    SceneRenderer& operator=(const SceneRenderer&) = delete;
    SceneRenderer(SceneRenderer&&) noexcept = default;
    SceneRenderer& operator=(SceneRenderer&&) noexcept = default;

    void Setup(std::span<uint32_t> passIDs);
    void Render(const Scene& scene, std::span<uint32_t> passIDs);

    PipelineManager* GetPipelineManager() { return m_pipelineManager.get(); }
    ShaderManager* GetShaderManager() { return m_shaderManager.get(); }
    TextureManager* GetTextureManager() { return m_textureManager.get(); }
    MaterialManager* GetMaterialManager() { return m_materialManager.get(); }
    MeshManager* GetMeshManager() { return m_meshManager.get(); }
    VertexLayoutManager* GetVertexLayoutManager() { return m_vertexLayoutManager.get(); }
    LayoutCache* GetLayoutCache() { return m_layoutCache.get(); }
    PassManager* GetPassManager() { return m_passManager.get(); }
    BindGroupManager* GetBindGroupManager() { return m_bindGroupManager.get(); }
    RenderGraph* GetRenderGraph() { return &m_renderGraph; }

  private:
    Device* m_device;
    AssetManager* m_assetManager;

    std::unique_ptr<LayoutCache> m_layoutCache;
    std::unique_ptr<VertexLayoutManager> m_vertexLayoutManager;
    std::unique_ptr<PassManager> m_passManager;
    std::unique_ptr<TextureManager> m_textureManager;
    std::unique_ptr<MaterialManager> m_materialManager;
    std::unique_ptr<MeshManager> m_meshManager;
    std::unique_ptr<PipelineManager> m_pipelineManager;
    std::unique_ptr<ShaderManager> m_shaderManager;
    std::unique_ptr<BindGroupManager> m_bindGroupManager;
    RenderGraph m_renderGraph;
    RenderQueue m_renderQueue;

    wgpu::BindGroup m_globalBindGroup;
    wgpu::Buffer m_globalUniformBuffer;
    wgpu::Sampler m_linearRepeatSampler;
    wgpu::Sampler m_pointSampler;
    wgpu::Texture m_depthTexture;
    TransientResourcePool m_vra;
    CompiledGraph m_compiledGraph;

    void Prepare(CompiledGraph& compiledGraph, RenderQueue& renderQueue);
    void Execute(CompiledGraph& compiledGraph, RenderQueue& renderQueue);
};

}  // namespace core::render
