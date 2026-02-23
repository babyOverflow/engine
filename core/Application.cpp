#include "Application.h"
#include <ranges>

namespace core {

wgpu::BindGroupLayoutDescriptor Application::GetGlobalLayouDesc() {
    static const std::array<wgpu::BindGroupLayoutEntry, 1> entries{wgpu::BindGroupLayoutEntry{
        .binding = 0,
        .visibility = wgpu::ShaderStage::Vertex,
        .buffer =
            wgpu::BufferBindingLayout{
                .type = wgpu::BufferBindingType::Uniform,
                .hasDynamicOffset = false,
                .minBindingSize = 0,
            },
    }};
    return wgpu::BindGroupLayoutDescriptor{
        .entryCount = entries.size(),
        .entries = entries.data(),
    };
}
std::expected<Application, int> core::Application::Create(ApplicationSpec& spec) {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << '\n';
        return std::unexpected(-1);
    }

    auto eventDispatcher = std::make_unique<EventDispatcher>();
    spec.winSpec->eventDispatcher = eventDispatcher.get();

    auto re = core::Window::Create(*spec.winSpec);

    if (!re.has_value()) {
        return std::unexpected(std::move(re.error()));
    }

    Window window = std::move(re.value());
    std::unique_ptr<render::Device> device = render::Device::Create(window);
    auto assetManager = std::make_unique<AssetManager>(AssetManager::Create(device.get()));
    auto globalBindGroupLayout = GetGlobalLayouDesc();
    auto layoutCache = std::make_unique<render::LayoutCache>(device.get());
    auto pipelineManager = std::make_unique<render::PipelineManager>(
        device.get(), layoutCache.get(), globalBindGroupLayout);
    auto shaderManager = std::make_unique<render::ShaderManager>(device.get(), assetManager.get(),
                                                                 layoutCache.get());
    auto meshManager = std::make_unique<render::MeshManager>(device.get(), assetManager.get());
    auto textureManager =
        std::make_unique<render::TextureManager>(device.get(), assetManager.get());
    auto materialManager = std::make_unique<render::MaterialManager>(
        device.get(), assetManager.get(), shaderManager.get(), textureManager.get(),
        layoutCache.get());

    render::RenderGraph renderGraph(device.get(), pipelineManager.get(),
                                    layoutCache->GetBindGroupLayout(globalBindGroupLayout));

    return Application(std::move(window), std::move(device), std::move(assetManager),
                       std::move(eventDispatcher), std::move(renderGraph), std::move(layoutCache),
                       std::move(pipelineManager), std::move(shaderManager),
                       std::move(textureManager), std::move(materialManager),
                       std::move(meshManager));
}

core::Application::~Application() {}

void core::Application::Run() {
    render::FrameContext frameContext;
    while (!m_souldColose) {
        m_window.PollEvent();
        m_eventDispatcher->ProcessEvent([this](auto&& event) -> void { this->RaiseEvent(event); });

        for (auto& layer : m_Layers) {
            // layer->OnUpdate(); // Assuming Layer has an OnUpdate method
            layer->OnUpdate();
        }

        for (auto& layer : m_Layers) {
            layer->OnRender(frameContext);
        }
        m_renderGraph.Execute(frameContext);
        m_device->Present();

        frameContext.ClearQueue();
    }

    glfwTerminate();
}

void core::Application::AttachLayer(std::unique_ptr<Layer> layer) {
    layer->OnAttach();
    m_Layers.emplace_back(std::move(layer));
}

void Application::RaiseEvent(Event& event) {
    for (auto& layer : std::views::reverse(m_Layers)) {
        bool catched = layer->OnEvent(event);
        if (catched) {
            return;
        }
    }
}
}  // namespace core