#include <slang.h>
#include <array>
#include <glm/glm.hpp>
#include <print>
#include <string_view>

#include "ExampleLayer.h"
#include "util.h"

using core::render::GpuBindGroup;
using core::render::GpuBindGroupLayout;
using core::render::GpuBuffer;
using core::render::GpuPipelineLayout;

struct Uniform {
    glm::mat4x4 modelViewProjection;
};

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 uv;
    glm::vec4 tangent;
};

std::unique_ptr<ExampleLayer> ExampleLayer::Create(core::Application* app) {
    using core::render::Vertex;

    core::render::Device* device = app->GetDevice();
    core::render::PipelineManager* pipelineManager = app->GetPipelineManager();
    core::AssetManager* assetManager = app->GetAssetManager();

    using namespace core::render;

    auto vsHandle =  assetManager->LoadShader("assets/tri_VS.shdr");
    auto fsHandle =  assetManager->LoadShader("assets/tri_FS.shdr");

    core::render::PipelineDesc pipelineDesc{
        .vertexShader = assetManager->GetShaderModule(vsHandle),
        .fragmentShader = assetManager->GetShaderModule(fsHandle),
        .vertexType = core::render::VertexType::StandardMesh,
        .blendState = wgx::BlendState::kReplace,
    };
    const GpuRenderPipeline* renderPipeline = pipelineManager->GetRenderPipeline(pipelineDesc);

    auto config = device->GetSurfaceConfig();
    auto proj = common::Projection::Perspective(45, config.width, config.height, 0.1, 100.0);
    common::GameCamera gameCamera{glm::vec3(0.F, 0.F, 0.F), 0.F, 0.F, proj};

    return std::unique_ptr<ExampleLayer>(new ExampleLayer(app, device, gameCamera, renderPipeline));
}

void ExampleLayer::OnAttach() {
    auto assetManager = m_app->GetAssetManager();
    m_modelHandle = assetManager->LoadModel("resources/microphone/scene.gltf");
    if (!m_modelHandle.IsValid()) {
        std::println("failed to load");
    }
}

void ExampleLayer::OnRender(core::render::FrameContext& context) {
    auto& d = m_device->GetDeivce();
    auto am = m_app->GetAssetManager();
    auto model = am->GetModel(m_modelHandle);

    auto cameraData = m_gameCamera.GetCameraUniformData();
    context.SetCameraData(cameraData);

    for (auto& mesh : model->subMeshes) {
        core::render::RenderPacket packet{.pipeline = m_renderPipeline->GetHandle(),
                                          .vertexBuffer = mesh.vertexBuffer.GetHandle(),
                                          .indexBuffer = mesh.indexBuffer.GetHandle(),
                                          .indexCount = mesh.indexCount};
        context.Submit(packet);
    }
}

void ExampleLayer::OnUpdate() {
    m_cameraController.UpdateCamera(m_gameCamera, 0.1);
}

bool ExampleLayer::OnEvent(core::Event& event) {
    return std::visit(core::overloaded{[&](core::event::KeyPressEvent& event) {
                                           m_cameraController.OnKeyboardPress(event);
                                           return true;
                                       },
                                       [&](core::event::KeyReleaseEvent& event) {
                                           m_cameraController.OnKeyboardRelease(event);
                                           return true;
                                       },
                                       [&](core::event::CursorPosEvent& event) {
                                           m_cameraController.OnMouseMove(event);
                                           return true;
                                       },
                                       [](core::event::WindowCloseEvent& event) {
                                           exit(0);
                                           return true;
                                       },
                                       [](auto& e) { return false; }},
                      event);
}
