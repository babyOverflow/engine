#pragma once
#include <webgpu/webgpu_cpp.h>
#include <span>
#include "../graph/IRenderPass.h"
#include "../render.h"
#include "AssetManager.h"

namespace core::render::pass {
class DeferredLightingPass : public core::render::IRenderPass {
  public:
    DeferredLightingPass() = default;
    ~DeferredLightingPass() {}
    void Execute(wgpu::RenderPassEncoder encoder, const PassExecuteContext& executeContext) override;

    const core::render::PassTargetState& GetTargetState() const override { return kSignature; }

    void Setup(PassSetupContext& context) override;

    std::string GetPassName() override { return "DeferredLightingPass"; }

  private:
    static const core::render::PassTargetState kSignature;
};
}  // namespace core::render::pass