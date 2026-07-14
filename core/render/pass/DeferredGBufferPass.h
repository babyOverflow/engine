#pragma once
#include <webgpu/webgpu_cpp.h>
#include "../graph/IRenderPass.h"

namespace core::render::pass {
class DeferredGBufferPass : public core::render::IRenderPass {
  public:
    DeferredGBufferPass() {};
    void Execute(wgpu::RenderPassEncoder encoder,
                 const PassExecuteContext& executeContext) override;

    void Setup(PassSetupContext& context) override;
};
}  // namespace core::render::pass
