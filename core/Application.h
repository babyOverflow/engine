#pragma once
#include <memory>
#include <vector>

#include "AssetManager.h"
#include "Layer.h"
#include "Window.h"
#include "render/SceneRenderer.h"
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
    render::PipelineManager* GetPipelineManager() { return m_sceneRenderer->GetPipelineManager(); }
    render::ShaderManager* GetShaderManager() { return m_sceneRenderer->GetShaderManager(); }
    render::TextureManager* GetTextureManager() { return m_sceneRenderer->GetTextureManager(); }
    render::MaterialManager* GetMaterialManager() { return m_sceneRenderer->GetMaterialManager(); }
    render::MeshManager* GetMeshManager() { return m_sceneRenderer->GetMeshManager(); }
    render::Device* GetDevice() { return m_device.get(); }

    static wgpu::BindGroupLayoutDescriptor GetGlobalLayouDesc();

    const wgpu::BindGroupLayout GetGlobalLayout() {
        auto desc = GetGlobalLayouDesc();
        return m_sceneRenderer->GetLayoutCache()->GetBindGroupLayout(desc);
    }

  private:
    Application(Window window,
                std::unique_ptr<render::Device> device,
                std::unique_ptr<AssetManager> assetManager,
                std::unique_ptr<EventDispatcher> eventDispatcher,
                std::unique_ptr<render::SceneRenderer> sceneRenderer)
        : m_window(std::move(window)),
          m_device(std::move(device)),
          m_assetManager(std::move(assetManager)),
          m_eventDispatcher(std::move(eventDispatcher)),
          m_sceneRenderer(std::move(sceneRenderer)) {}

    Window m_window;
    std::unique_ptr<render::Device> m_device;
    std::unique_ptr<AssetManager> m_assetManager;
    std::unique_ptr<EventDispatcher> m_eventDispatcher;

    std::unique_ptr<render::SceneRenderer> m_sceneRenderer;
    Scene m_scene;

    std::vector<std::unique_ptr<Layer>> m_Layers;
    bool m_souldColose = false;
};
}  // namespace core
