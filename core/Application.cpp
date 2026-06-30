#include "Application.h"
#include <ranges>
#include "render/pass/DeferredGBufferPass.h"
#include "render/pass/DeferredLightingPass.h"
#include "render/pass/ForwardRenderPass.h"

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
    auto assetManager = std::make_unique<AssetManager>(AssetManager::Create(device.get()));
    auto globalBindGroupLayout = GetGlobalLayouDesc();
    auto layoutCache = std::make_unique<render::LayoutCache>(device.get());
    auto vertexLayoutManager = std::make_unique<render::VertexLayoutManager>();
    auto passManager = std::make_unique<render::PassManager>();

    passManager->RegisterPass<render::pass::ForwardRenderPass>("ForwardRenderPass");
    passManager->RegisterPass<render::pass::DeferredGBufferPass>("DeferredGBufferPass");
    passManager->RegisterPass<render::pass::DeferredLightingPass>("DeferredLightingPass");

    auto pipelineManager = std::make_unique<render::PipelineManager>(
        device.get(), layoutCache.get(), passManager.get(), vertexLayoutManager.get(),
        globalBindGroupLayout);
    auto meshManager = std::make_unique<render::MeshManager>(device.get(), assetManager.get(),
                                                             vertexLayoutManager.get());
    auto textureManager =
        std::make_unique<render::TextureManager>(device.get(), assetManager.get());
    auto materialManager = std::make_unique<render::MaterialManager>(
        device.get(), assetManager.get(), textureManager.get(), passManager.get(),
        layoutCache.get());

    auto shaderManager =
        std::make_unique<render::ShaderManager>(device.get(), assetManager.get(), layoutCache.get(),
                                                passManager.get(), materialManager.get());
    auto bindGroupManager = std::make_unique<render::BindGroupManager>(
        device.get(), layoutCache.get(), shaderManager.get(), materialManager.get());

    render::RenderGraph renderGraph(device.get(), assetManager.get(), shaderManager.get(),
                                    pipelineManager.get(),
                                    layoutCache->GetBindGroupLayout(globalBindGroupLayout));

    return Application(std::move(window), std::move(device), std::move(assetManager),
                       std::move(eventDispatcher), std::move(renderGraph),
                       std::move(vertexLayoutManager), std::move(layoutCache),
                       std::move(passManager), std::move(pipelineManager), std::move(shaderManager),
                       std::move(textureManager), std::move(materialManager),
                       std::move(meshManager), std::move(bindGroupManager));
}

core::Application::~Application() {}

void core::Application::Run() {
    render::RenderQueue renderQueue;

    // std::vector<uint32_t> passIDs{m_passManager->GetPassID("ForwardRenderPass")};
    std::vector<uint32_t> passIDs{
        //m_passManager->GetPassID("ForwardRenderPass"),
         m_passManager->GetPassID("DeferredGBufferPass"),
         m_passManager->GetPassID("DeferredLightingPass"),
    };
    std::vector<core::render::IRenderPass*> s =
        passIDs |
        std::views::transform([&](uint32_t passID) { return m_passManager->GetPass(passID); }) |
        std::ranges::to<std::vector>();

    m_renderGraph.Setup(passIDs, m_passManager.get());

    while (!m_souldColose) {
        m_window.PollEvent();
        m_eventDispatcher->ProcessEvent([this](auto&& event) -> void { this->RaiseEvent(event); });

        renderQueue.Clear();
        for (auto& layer : m_Layers) {
            // layer->OnUpdate(); // Assuming Layer has an OnUpdate method
            layer->OnUpdate(m_scene);
        }

        std::span<Handle> dirties = m_materialManager->GetDirtyMaterials();
        for (auto handle : dirties) {
            m_bindGroupManager->UpdateBindGroup(handle);
        }
        m_materialManager->ClearDirties();

        render::SceneCuller::ExtractRenderQueue(m_scene, passIDs, m_assetManager.get(),
                                                m_shaderManager.get(), m_pipelineManager.get(),
                                                m_bindGroupManager.get(), renderQueue);
        m_renderGraph.Prepare(renderQueue, m_pipelineManager.get());
        m_renderGraph.Execute(renderQueue);
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