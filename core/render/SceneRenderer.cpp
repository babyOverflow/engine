
#include "SceneRenderer.h"
#include "BindGroupManager.h"
#include "PipelineManager.h"
void core::render::SceneCuller::ExtractRenderQueue(const Scene& scene,
                                                   std::span<uint32_t> passes,
                                                   AssetManager* assetManager,
                                                   ShaderManager* shaderManager,
                                                   PipelineManager* pipelineManager,
                                                   BindGroupManager* bindGroupManager,
                                                   RenderQueue& outRenderQueue) {
    auto assetRegistry = assetManager->GetRegistry();
    for (uint32_t i = 0; i < scene.models.size(); ++i) {
        for (const auto& renderUnit : scene.models[i]->renderUnits) {
            AssetView<Material> material = assetManager->GetMaterial(renderUnit.materialHandle);
            for (uint32_t passId : passes) {
                PipelineKey key;
                Handle shaderHandle =
                    shaderManager->GetShaderHandle(passId, material->GetActiveTechniqueID());
                if (!shaderHandle.IsValid()) {
                    continue;
                }
                key.bits.shaderId = shaderHandle.index;
                key.bits.layoutId = assetManager->GetMesh(renderUnit.meshHandle)
                                        ->GetGlobalVertexStateID(renderUnit.subMeshIndex);
                key.bits.blendId = static_cast<uint64_t>(
                    BlendMode::Opaque);  // TODO: support blend mode in material
                key.bits.depthStencilId =
                    DepthStencilStateManager::kDefaultDepthStateID;  // TODO: support depth
                                                                     // state in material
                key.bits.passId = passId;
                key.bits.topology = static_cast<uint64_t>(wgpu::PrimitiveTopology::TriangleList);
                key.bits.cullMode = static_cast<uint64_t>(wgpu::CullMode::Back);
                key.bits.frontFace = static_cast<uint64_t>(wgpu::FrontFace::CCW);

                RenderIntent intent;
                intent.meshHandle = renderUnit.meshHandle;
                intent.subMeshIndex = renderUnit.subMeshIndex;
                intent.materialHandle = renderUnit.materialHandle;
                intent.transformIndex = i;
                intent.pipelineKey = key;
                intent.sortKey.bits.pipelineId =
                    pipelineManager->GetPipelineID(key, assetRegistry).index;
                intent.sortKey.bits.passId = passId;

                intent.bindGroup = bindGroupManager->GetBindGroup(material.handle); // GetBindGroup!

                outRenderQueue.renderIntents.push_back(intent);
            }
        }
    }

    outRenderQueue.cameraData = scene.cameraData;
    outRenderQueue.transforms = std::span(scene.modelMatrices.data(), scene.modelMatrices.size());
}