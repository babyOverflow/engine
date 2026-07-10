#pragma once
#include <dawn/webgpu_cpp.h>
#include <algorithm>
#include <vector>

namespace wgx {
struct VertexAttribute {
    wgpu::VertexFormat format = {};
    uint64_t offset;
    uint32_t shaderLocation;

    bool operator==(const VertexAttribute&) const = default;
};

struct VertexBufferLayout {
    wgpu::VertexStepMode stepMode = wgpu::VertexStepMode::Undefined;
    uint64_t arrayStride;
    std::vector<VertexAttribute> attributes;

    bool operator==(const VertexBufferLayout& other) const {
        if (stepMode != other.stepMode || arrayStride != other.arrayStride) {
            return false;
        }
        return std::ranges::equal(attributes, other.attributes);
    }
};

// bool operator==(const wgpu::StencilFaceState& lhs, const wgpu::StencilFaceState& rhs) {
//     return lhs.compare == rhs.compare && lhs.failOp == rhs.failOp &&
//            lhs.depthFailOp == rhs.depthFailOp && lhs.passOp == rhs.passOp;
// }

// struct DepthStencilState {
//     wgpu::TextureFormat format = wgpu::TextureFormat::Undefined;
//     wgpu::OptionalBool depthWriteEnabled = wgpu::OptionalBool::Undefined;
//     wgpu::CompareFunction depthCompare = wgpu::CompareFunction::Undefined;
//     wgpu::StencilFaceState stencilFront = {};
//     wgpu::StencilFaceState stencilBack = {};
//     uint32_t stencilReadMask = 0xFFFFFFFF;
//     uint32_t stencilWriteMask = 0xFFFFFFFF;
//     int32_t depthBias = 0;
//     float depthBiasSlopeScale = 0.f;
//     float depthBiasClamp = 0.f;
//
//     bool operator==(const DepthStencilState& other) const {
//         return format == other.format && depthWriteEnabled == other.depthWriteEnabled &&
//                depthCompare == other.depthCompare && stencilFront == other.stencilFront &&
//                stencilBack == other.stencilBack && stencilReadMask == other.stencilReadMask &&
//                stencilWriteMask == other.stencilWriteMask && depthBias == other.depthBias &&
//                depthBiasSlopeScale == other.depthBiasSlopeScale &&
//                depthBiasClamp == other.depthBiasClamp;
//     }
//
// };
}  // namespace wgx
