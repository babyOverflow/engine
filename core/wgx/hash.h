#pragma once
#include <dawn/webgpu_cpp.h>

namespace wgx {

inline void hash_combine(std::size_t& seed, std::size_t value) {
    seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

inline bool Equals(const wgpu::PrimitiveState& a, const wgpu::PrimitiveState& b) {
    return (a.topology == b.topology && a.stripIndexFormat == b.stripIndexFormat &&
            a.frontFace == b.frontFace && a.cullMode == b.cullMode &&
            a.unclippedDepth == b.unclippedDepth);
}

inline std::size_t Hash(const wgpu::PrimitiveState& s) {
    std::size_t seed = 0;

    hash_combine(seed, std::hash<int>{}(static_cast<int>(s.topology)));
    hash_combine(seed, std::hash<int>{}(static_cast<int>(s.frontFace)));
    hash_combine(seed, std::hash<int>{}(static_cast<int>(s.stripIndexFormat)));
    hash_combine(seed, std::hash<int>{}(static_cast<int>(s.cullMode)));

    seed += static_cast<std::size_t>(s.unclippedDepth);
    return seed;
}

inline bool Equals(const wgpu::BlendComponent& a, const wgpu::BlendComponent& b) {
    return a.dstFactor == b.dstFactor && a.operation == b.operation && a.srcFactor == b.srcFactor;
}

inline std::size_t Hash(const wgpu::BlendComponent& s) {
    std::size_t seed = 0;

    hash_combine(seed, std::hash<int>{}(static_cast<int>(s.dstFactor)));
    hash_combine(seed, std::hash<int>{}(static_cast<int>(s.srcFactor)));
    hash_combine(seed, std::hash<int>{}(static_cast<int>(s.operation)));

    return seed;
}

inline bool Equals(const wgpu::BlendState& a, const wgpu::BlendState& b) {
    return Equals(a.alpha, b.alpha) && Equals(a.color, b.color);
}

inline std::size_t Hash(const wgpu::BlendState& s) {
    std::size_t seed = 0;

    hash_combine(seed, Hash(s.alpha));
    hash_combine(seed, Hash(s.color));

    return seed;
}

// TODO!(Stencil comparison is not implemented, before implement stencil it must be implemented)
inline bool Equals(const wgpu::DepthStencilState& a, const wgpu::DepthStencilState& b) {
    if (a.format != b.format || a.depthWriteEnabled != b.depthWriteEnabled ||
        a.depthCompare != b.depthCompare) {
        return false;
    }

    if (a.depthBias != b.depthBias || a.depthBiasSlopeScale != b.depthBiasSlopeScale ||
        a.depthBiasClamp != b.depthBiasClamp) {
        return false;
    }

    return true;
}

// TODO!(Stencil comparison is not implemented, before implement stencil it must be implemented)
inline std::size_t Hash(const wgpu::DepthStencilState& s) {
    std::size_t seed = 0;

    hash_combine(seed, std::hash<int>{}(static_cast<int>(s.format)));
    hash_combine(seed, std::hash<bool>{}(s.depthWriteEnabled));
    hash_combine(seed, std::hash<int>{}(static_cast<int>(s.depthCompare)));

    hash_combine(seed, std::hash<float>{}(s.depthBias));
    hash_combine(seed, std::hash<float>{}(s.depthBiasSlopeScale));
    hash_combine(seed, std::hash<float>{}(s.depthBiasClamp));

    return seed;
}

}  // namespace wgx