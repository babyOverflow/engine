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

    using namespace core::render;
     //const auto code =
     //    core::util::ReadFileToString("shaders/tri.wgsl");
     //if (!code.has_value()) {
     //    return nullptr;
     //}
     //GpuShaderModule shader = device->CreateShaderModuleFromWGSL(code.value());
    auto shaderAssetOrError =
        core::util::ReadFileToByte("assets/tri_VS.shdr").and_then(core::ShaderAsset::LoadFromMemory);
    if (!shaderAssetOrError.has_value()) {
        return nullptr;
    }
    auto fragShaderAssetOrError =
        core::util::ReadFileToByte("assets/tri_FS.shdr").and_then(core::ShaderAsset::LoadFromMemory);
    core::ShaderAsset shaderAsset = shaderAssetOrError.value();
    core::ShaderAsset fragShaderAsset = fragShaderAssetOrError.value();

    std::string_view wgslCode = std::string_view(
        reinterpret_cast<const char*>(shaderAsset.code.data()), shaderAsset.code.size());
    GpuShaderModule shader = device->CreateShaderModuleFromWGSL(wgslCode);

    std::string_view fragWgslCode = std::string_view(
        reinterpret_cast<const char*>(fragShaderAsset.code.data()), fragShaderAsset.code.size());
    GpuShaderModule fragShader = device->CreateShaderModuleFromWGSL(fragWgslCode);

    GpuPipelineLayout renderPipelineLayout =
        device->CreatePipelineLayout(wgpu::PipelineLayoutDescriptor{
            .bindGroupLayoutCount = 1, .bindGroupLayouts = &app->GetGlobalLayout().GetHandle()});

    std::array<wgpu::VertexBufferLayout, 1> vertexBufferLayouts{
        wgpu::VertexBufferLayout{.arrayStride = sizeof(Vertex),
                                 .attributeCount = Vertex::GetAttributes().size(),
                                 .attributes = Vertex::GetAttributes().data()},
    };

    wgpu::ColorTargetState targets{.format = device->GetSurfaceConfig().format,
                                   .blend = &wgx::BlendState::kReplace,
                                   .writeMask = wgpu::ColorWriteMask::All};
    wgpu::FragmentState fragment = wgpu::FragmentState{.module = fragShader.GetHandle(),
                                                       .entryPoint = "fragmentMain",
                                                       .targetCount = 1,
                                                       .targets = &targets};
    GpuRenderPipeline renderPipeline = device->CreateRenderPipeline(wgpu::RenderPipelineDescriptor{
        .layout = renderPipelineLayout.GetHandle(),
        .vertex = wgpu::VertexState{.module = shader.GetHandle(),
                                    .entryPoint = "vertexMain",
                                    .bufferCount = vertexBufferLayouts.size(),
                                    .buffers = vertexBufferLayouts.data()},
        .primitive = wgx::PrimitiveState::kDefault,
        .fragment = &fragment,
    });

    auto config = device->GetSurfaceConfig();
    auto proj = common::Projection::Perspective(45, config.width, config.height, 0.1, 100.0);
    common::GameCamera gameCamera{glm::vec3(0.F, 0.F, 0.F), 0.F, 0.F, proj};

    std::vector<core::render::GpuShaderModule> shaders;
    
    shaders.emplace_back(std::move(shader));
    shaders.emplace_back(std::move(fragShader));
    return std::unique_ptr<ExampleLayer>(
        new ExampleLayer(app, device, gameCamera, std::move(shaders),
                         std::move(renderPipelineLayout), std::move(renderPipeline)));
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
        core::render::RenderPacket packet{.pipeline = m_renderPipeline.GetHandle(),
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
