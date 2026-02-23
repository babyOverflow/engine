#include <slang.h>
#include <array>
#include <glm/glm.hpp>
#include <print>
#include <string_view>

#include "ExampleLayer.h"
#include "util.h"

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
    core::render::ShaderManager* shaderManager = app->GetShaderManager();

    using namespace core::render;

    auto shader = shaderManager->GetStandardShader();

    core::render::PipelineDesc pipelineDesc{
        .shaderAsset = shader,
        .vertexType = core::render::VertexType::StandardMesh,
        .blendState = wgx::BlendState::kReplace,
    };
    pipelineManager->GetRenderPipeline(pipelineDesc);

    auto config = device->GetSurfaceConfig();
    auto proj = common::Projection::Perspective(45, config.width, config.height, 0.1, 100.0);
    common::GameCamera gameCamera{glm::vec3(0.F, 0.F, 0.F), 0.F, 0.F, proj};

    loader::GLTFLoader loader{
        app->GetAssetManager(),
        app->GetTextureManager(),
         app->GetMaterialManager(),
        app->GetMeshManager(),
    };

    return std::unique_ptr<ExampleLayer>(
        new ExampleLayer(app, device, gameCamera,  loader));
}

void ExampleLayer::OnAttach() {
    auto modelOrError = m_loader.LoadModel("resources/microphone/scene.gltf");
    if (!modelOrError.has_value()) {
        std::println("failed to load");
    }
    m_modelHandle = modelOrError.value();
}

void ExampleLayer::OnRender(core::render::FrameContext& context) {
    auto& d = m_device->GetDeivce();
    auto am = m_app->GetAssetManager();
    auto modelView = am->GetModel(m_modelHandle);

    auto cameraData = m_gameCamera.GetCameraUniformData();
    context.SetCameraData(cameraData);

    for (auto& renderUnit : modelView->renderUnits) {
        auto meshView = am->GetMesh(renderUnit.meshHandle);
        auto mesh = meshView->submeshInfos[renderUnit.subMeshIndex];

        auto modelMatrix = renderUnit.modelMatrix;
        auto material = am->GetMaterial(renderUnit.materialHandle);

        core::render::RenderPacket packet{
            .vertexBuffer = meshView->vertexBuffer,
            .indexBuffer = meshView->indexBuffer,
            .indexCount = mesh.indexCount,
            .material = material,
        };
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
