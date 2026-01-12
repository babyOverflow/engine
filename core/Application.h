#pragma once
#include <memory>
#include <vector>

#include "Layer.h"
#include "Window.h"
#include "render/render.h"
#include "AssetManager.h"

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

    AssetManager* GetAssetManager() { return &m_assetManager; }
    render::Device* GetDevice() { return m_device.get(); }

    const render::GpuBindGroupLayout& GetGlobalLayout() const { return m_globalBindGroupLayout; }

  private:
    Application(Window window, std::unique_ptr<render::Device> device, AssetManager&& assetManager, std::unique_ptr<EventDispatcher> eventDispatcher, render::RenderGraph renderGraph, render::GpuBindGroupLayout bindGroupLayout)
        : m_window(std::move(window)), m_device(std::move(device)), m_assetManager(std::move(assetManager)), m_eventDispatcher(std::move(eventDispatcher)), m_renderGraph(std::move(renderGraph)), m_globalBindGroupLayout(std::move(bindGroupLayout)) {}

    Window m_window;
    std::unique_ptr<render::Device> m_device;
    AssetManager m_assetManager;
    std::unique_ptr<EventDispatcher> m_eventDispatcher;

    // TODO!(make RenderSystem and replace these variables);
    render::GpuBindGroupLayout m_globalBindGroupLayout;
    render::RenderGraph m_renderGraph;

    std::vector<std::unique_ptr<Layer>> m_Layers;
    bool m_souldColose = false;
};
}  // namespace core