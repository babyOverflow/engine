#pragma once
#include <dawn/webgpu_cpp.h>

namespace wgx {

namespace BlendComponent {
inline const wgpu::BlendComponent kReplace{
    .operation = wgpu::BlendOperation::Add,
    .srcFactor = wgpu::BlendFactor::One,
    .dstFactor = wgpu::BlendFactor::Zero,
};
}

namespace BlendState {
inline const wgpu::BlendState kReplace{
    .color = BlendComponent::kReplace,
    .alpha = BlendComponent::kReplace,
};

}

namespace PrimitiveState {
inline const wgpu::PrimitiveState kDefault{.topology = wgpu::PrimitiveTopology::TriangleList,
                              .stripIndexFormat = wgpu::IndexFormat::Undefined,
                              .frontFace = wgpu::FrontFace::CCW,
                              .cullMode = wgpu::CullMode::Back};
}

}

