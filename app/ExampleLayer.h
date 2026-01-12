#pragma once
#include "Application.h"
#include "Event.h"
#include "Layer.h"
#include "render/render.h"
#include "ShaderAssetFormat.h"


#include "Camera3D.h"

class ExampleLayer : public core::Layer {
  public:
    static std::unique_ptr<ExampleLayer> Create(core::Application* app);

    void OnAttach() override;
    void OnRender(core::render::FrameContext& context) override;
    void OnUpdate() override;
    bool OnEvent(core::Event& event) override;

  private:
    ExampleLayer(core::Application* app,
                 core::render::Device* device,
                 common::GameCamera camera,
                 std::vector<core::render::GpuShaderModule> shaderModules,
                 core::render::GpuPipelineLayout pipelineLayout,
                 core::render::GpuRenderPipeline renderPipeline

                 )
        : m_app(app),
          m_gameCamera(camera),
          m_device(device),
          m_shaderModule(std::move(shaderModules)),
          m_renderPipelineLayout(std::move(pipelineLayout)),
          m_renderPipeline(std::move(renderPipeline))
    {}

    common::GameCamera m_gameCamera;
    common::CameraController m_cameraController;

    core::Application* m_app;
    core::render::Device* m_device;
    std::vector<core::render::GpuShaderModule> m_shaderModule;
    core::render::GpuPipelineLayout m_renderPipelineLayout;
    core::render::GpuRenderPipeline m_renderPipeline;

    core::Handle m_modelHandle;
};
