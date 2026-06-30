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

    const core::render::PassTargetState& GetTargetState() const override { return kSignature; }

    void Execute(wgpu::RenderPassEncoder encoder,
                 
                 const PassExecuteContext& executeContext) override;

    std::string GetPassName() override { return "ForwardRenderPass"; }

  private:
    static const core::render::PassTargetState kSignature;
};
}  // namespace core::render::pass