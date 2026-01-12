#include "Application.h"
#include <ranges>

namespace core {

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
    auto assetManager = AssetManager::Create(device.get());

    const std::array<wgpu::BindGroupLayoutEntry, 1> entries{wgpu::BindGroupLayoutEntry{
        .binding = 0,
        .visibility = wgpu::ShaderStage::Vertex,
        .buffer =
            wgpu::BufferBindingLayout{
                .type = wgpu::BufferBindingType::Uniform,
                .hasDynamicOffset = false,
                .minBindingSize = 0,
            },
    }};
    render::GpuBindGroupLayout globalBindGroupLayout =
        device->CreateBindGroupLayout(wgpu::BindGroupLayoutDescriptor{
            .entryCount = entries.size(),
            .entries = entries.data(),
        });
    render::RenderGraph renderGraph(device.get(), globalBindGroupLayout);

    return Application(std::move(window), std::move(device), std::move(assetManager),
                       std::move(eventDispatcher), std::move(renderGraph),
                       std::move(globalBindGroupLayout));
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