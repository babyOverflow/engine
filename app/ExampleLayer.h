#pragma once
#include "Application.h"
#include "Event.h"
#include "Layer.h"
#include "ModelLoader.h"
#include "ShaderAssetFormat.h"
#include "render/render.h"

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
                 loader::GLTFLoader loader)
        : m_app(app),
          m_gameCamera(camera),
          m_device(device),
          m_loader(loader) {}

    common::GameCamera m_gameCamera;
    common::CameraController m_cameraController;

    core::Application* m_app;
    core::render::Device* m_device;
    loader::GLTFLoader m_loader;

    core::Handle m_modelHandle;
};
