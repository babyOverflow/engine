#pragma once
#include <memory>
#include <vector>

#include "AssetManager.h"
#include "Layer.h"
#include "Window.h"
#include "render/MeshManager.h"
#include "render/PipelineManager.h"
#include "render/ShaderSystem.h"
#include "render/TextureManager.h"
#include "render/render.h"

namespace core {

struct ApplicationSpec {
    WindowSpec* winSpec;
};
class Application {
  public:
    Application() = delete;
    static std::expected<Application, int> Create(ApplicationSpec& spec);
    ~Application();
    Application(Application&& other) = default;

    void Run();
    void AttachLayer(std::unique_ptr<Layer> layer);

    void RaiseEvent(Event& event);

    AssetManager* GetAssetManager() { return m_assetManager.get(); }
    render::PipelineManager* GetPipelineManager() { return m_pipelineManager.get(); }
    render::ShaderSystem* GetShaderManager() { return m_shaderManager.get(); }
    render::TextureManager* GetTextureManager() { return m_textureManager.get(); }
    render::MeshManager* GetMeshManager() { return m_meshManager.get(); }
    render::Device* GetDevice() { return m_device.get(); }

    static wgpu::BindGroupLayoutDescriptor GetGlobalLayouDesc();

    const wgpu::BindGroupLayout GetGlobalLayout() {
        auto desc = GetGlobalLayouDesc();
        return m_layoutCache->GetBindGroupLayout(desc);
    }

  private:
    Application(Window window,
                std::unique_ptr<render::Device> device,
                std::unique_ptr<AssetManager> assetManager,
                std::unique_ptr<EventDispatcher> eventDispatcher,
                render::RenderGraph renderGraph,
                std::unique_ptr<render::LayoutCache> layoutCache,
                std::unique_ptr<render::PipelineManager> pipelineManager,
                std::unique_ptr<render::ShaderSystem> shaderManager,
                std::unique_ptr<render::TextureManager> textureManager,
                std::unique_ptr<render::MeshManager> meshManager)
        : m_window(std::move(window)),
          m_device(std::move(device)),
          m_assetManager(std::move(assetManager)),
          m_eventDispatcher(std::move(eventDispatcher)),
          m_renderGraph(std::move(renderGraph)),
          m_layoutCache(std::move(layoutCache)),
          m_pipelineManager(std::move(pipelineManager)),
          m_shaderManager(std::move(shaderManager)),
          m_textureManager(std::move(textureManager)),
          m_meshManager(std::move(meshManager)) {}

    Window m_window;
    std::unique_ptr<render::Device> m_device;
    std::unique_ptr<AssetManager> m_assetManager;
    std::unique_ptr<EventDispatcher> m_eventDispatcher;

    // TODO!(make RenderSystem and replace these variables);
    std::unique_ptr<render::LayoutCache> m_layoutCache;
    std::unique_ptr<render::PipelineManager> m_pipelineManager;
    std::unique_ptr<render::TextureManager> m_textureManager;
    std::unique_ptr<render::ShaderSystem> m_shaderManager;
    std::unique_ptr<render::MeshManager> m_meshManager;
    render::RenderGraph m_renderGraph;

    std::vector<std::unique_ptr<Layer>> m_Layers;
    bool m_souldColose = false;
};
}  // namespace core