#pragma once
#include <webgpu/webgpu_cpp.h>
#include "../graph/IRenderPass.h"

namespace core::render::pass {
class ForwardRenderPass : public core::render::IRenderPass {
  public:
    ForwardRenderPass() = default;

    void Setup(PassSetupContext& context) override;

    void Execute(wgpu::RenderPassEncoder encoder,

                 const PassExecuteContext& executeContext) override;
};
}  // namespace core::render::pass
