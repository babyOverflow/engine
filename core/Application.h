#pragma once
#include <memory>
#include <vector>

#include "AssetManager.h"
#include "Layer.h"
#include "Window.h"
#include "render/PipelineManager.h"
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
    render::Device* GetDevice() { return m_device.get(); }

    static wgpu::BindGroupLayoutDescriptor GetGlobalLayouDesc();

    const wgpu::BindGroupLayout GetGlobalLayout() {
        auto desc = GetGlobalLayouDesc();
        return m_layoutCache->GetBindGroupLayout(desc);
    }

  private:
    Application(Window window,
                std::unique_ptr<render::Device> device,
                std::unique_ptr < AssetManager> && assetManager,
                std::unique_ptr<EventDispatcher> eventDispatcher,
                render::RenderGraph renderGraph,
                std::unique_ptr<render::PipelineManager> pipelineManager,
                std::unique_ptr<render::LayoutCache> layoutCache,
                std::unique_ptr<render::ShaderSystem> shaderSystem,
                std::unique_ptr<render::MaterialSystem> materialSystem)
        : m_window(std::move(window)),
          m_device(std::move(device)),
          m_assetManager(std::move(assetManager)),
          m_eventDispatcher(std::move(eventDispatcher)),
          m_renderGraph(std::move(renderGraph)),
          m_layoutCache(std::move(layoutCache)),
          m_pipelineManager(std::move(pipelineManager)),
          m_shaderSystem(std::move(shaderSystem)),
          m_materialSystem(std::move(materialSystem)) {}

    Window m_window;
    std::unique_ptr<render::Device> m_device;
    std::unique_ptr < AssetManager> m_assetManager;
    std::unique_ptr<EventDispatcher> m_eventDispatcher;

    // TODO!(make RenderSystem and replace these variables);
    std::unique_ptr<render::LayoutCache> m_layoutCache;
    std::unique_ptr<render::PipelineManager> m_pipelineManager;
    render::RenderGraph m_renderGraph;
    std::unique_ptr<render::ShaderSystem> m_shaderSystem;
    std::unique_ptr<render::MaterialSystem> m_materialSystem;

    std::vector<std::unique_ptr<Layer>> m_Layers;
    bool m_souldColose = false;
};
}  // namespace core