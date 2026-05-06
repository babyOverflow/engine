#pragma once
#include <webgpu/webgpu_cpp.h>
#include <span>
#include "../IRenderPass.h"
#include "../render.h"
#include "AssetManager.h"

namespace core::render::pass {
class DeferredGBufferPass : public core::render::IRenderPass {
  public:
    DeferredGBufferPass() {};
    void Execute(wgpu::RenderPassEncoder encoder,
                 std::span<RenderIntent> context,
                 const AssetRegistry& assetRegistry,
                 std::span<const wgpu::RenderPipeline> pipelines) override;

    void Setup(PassSetupContext& context) override;

    std::string GetVertexEntryName() override { return "vertexDeferredGBuffer"; }
    std::string GetFragmentEntryName() override { return "fragmentDeferredGBuffer"; }
    std::string GetPassName() override { return "DeferredGBufferPass"; }

  private:
};
}  // namespace core::render::pass