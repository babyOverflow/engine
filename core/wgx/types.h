#pragma once
#include <dawn/webgpu_cpp.h>
#include <ranges>
#include <vector>
#include <algorithm>

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
}  // namespace wgx