#pragma once
#include <webgpu/webgpu_cpp.h>
#include <span>
#include "../IRenderPass.h"
#include "../render.h"
#include "AssetManager.h"

namespace core::render::pass {
//class DeferredLightingPass : public core::render::IRenderPass {
//  public:
//    DeferredLightingPass() = default;
//    ~DeferredLightingPass() {}
//    void Execute(wgpu::RenderPassEncoder encoder,
//                 std::span<RenderIntent> context,
//                 const AssetRegistry& assetRegistry,
//                 std::span<const wgpu::RenderPipeline> pipelines) override;
//
//    void Setup(PassSetupContext& context) override;
//
//    std::string GetVertexEntryName() override { return "vertexDeferredLighting"; }
//    std::string GetFragmentEntryName() override { return "fragmentDeferredLighting"; }
//    std::string GetPassName() override { return "DeferredLightingPass"; }
//
//  private:
//};
}  // namespace core::render::pass