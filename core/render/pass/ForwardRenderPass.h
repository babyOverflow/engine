#pragma once
#include <webgpu/webgpu_cpp.h>
#include <span>
#include "../IRenderPass.h"
#include "../render.h"
#include "AssetManager.h"

namespace core::render::pass {
class ForwardRenderPass : public core::render::IRenderPass {
  public:
    ForwardRenderPass() = default;



    void Setup(PassSetupContext& context) override;
    void Execute(wgpu::RenderPassEncoder encoder,
                 std::span<RenderIntent> context,
                 const AssetRegistry& assetRegistry,
                 std::span<const wgpu::RenderPipeline> pipelines) override;

    std::string GetPassName() override { return "ForwardRenderPass"; }

  private:
};
}  // namespace core::render::pass