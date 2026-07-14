#include "Application.h"
#include <ranges>

namespace core {

wgpu::BindGroupLayoutDescriptor Application::GetGlobalLayouDesc() {
    static const std::array<wgpu::BindGroupLayoutEntry, 3> entries{
        wgpu::BindGroupLayoutEntry{
            .binding = 0,
            .visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment,
            .buffer =
                wgpu::BufferBindingLayout{
                    .type = wgpu::BufferBindingType::Uniform,
                    .hasDynamicOffset = false,
                    .minBindingSize = 0,
                }},
        wgpu::BindGroupLayoutEntry{
            .binding = 1,
            .visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment,
            .sampler =
                wgpu::SamplerBindingLayout{
                    .type = wgpu::SamplerBindingType::Filtering,
                }},
        wgpu::BindGroupLayoutEntry{
            .binding = 2,
            .visibility = wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment,
            .sampler =
                wgpu::SamplerBindingLayout{
                    .type = wgpu::SamplerBindingType::NonFiltering,
                }},
    };

    return wgpu::BindGroupLayoutDescriptor{
        .label = "GlobalBindGroup",
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
    auto assetManager = std::make_unique<AssetManager>(AssetManager::Create());
    auto globalBindGroupLayout = GetGlobalLayouDesc();

    auto sceneRenderer = std::make_unique<render::SceneRenderer>(device.get(), assetManager.get(),
                                                                 globalBindGroupLayout);

    return Application(std::move(window), std::move(device), std::move(assetManager),
                       std::move(eventDispatcher), std::move(sceneRenderer));
}

core::Application::~Application() {}

void core::Application::Run() {
    auto passManager = m_sceneRenderer->GetPassManager();
    std::vector<uint32_t> passIDs{
        passManager->GetPassID("DeferredGBufferPass"),
        passManager->GetPassID("DeferredLightingPass"),
    };

    m_sceneRenderer->Setup(passIDs);

    while (!m_souldColose) {
        m_window.PollEvent();
        m_eventDispatcher->ProcessEvent([this](auto&& event) -> void { this->RaiseEvent(event); });

        for (auto& layer : m_Layers) {
            layer->OnUpdate(m_scene);
        }

        m_sceneRenderer->Render(m_scene, passIDs);
        m_device->Present();
    }

    glfwTerminate();
}

void core::Application::AttachLayer(std::unique_ptr<Layer> layer) {
    layer->OnAttach(m_scene);
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
