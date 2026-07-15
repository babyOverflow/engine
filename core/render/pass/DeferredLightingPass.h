#pragma once
#include <webgpu/webgpu_cpp.h>
#include "../graph/IRenderPass.h"

namespace core::render::pass {
class DeferredLightingPass : public core::render::IRenderPass {
  public:
    DeferredLightingPass() = default;
    ~DeferredLightingPass() {}
    void Execute(wgpu::RenderPassEncoder encoder,
                 const PassExecuteContext& executeContext) override;

    void Setup(PassSetupContext& context) override;
};
}  // namespace core::render::pass
