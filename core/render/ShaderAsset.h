#pragma once
#include <dawn/webgpu_cpp.h>
#include <array>
#include <expected>

#include "Common.h"
#include "ShaderAssetFormat.h"
#include "render.h"

namespace core::render {

struct BindingInfo {
    using Fmt = ShaderAssetFormat;
    uint32_t set;
    uint32_t binding;
    PropertyId id;
    Fmt::ResourceType resourceType;
    Fmt::ShaderVisibility visibility;
    Fmt::Resource resource = {.buffer = {0}};
};

struct MaterialVariableInfo {
    PropertyId id;
    size_t offset;
    size_t size;
};

struct ShaderReflectionData {
    std::vector<BindingInfo> bindings;
    std::vector<MaterialVariableInfo> materialVariables;
    size_t materialUniformSize = 0;
    ShaderAssetFormat::ShaderVisibility entryShaderStage;

    struct GroupRange {
        uint32_t offset = 0;
        uint32_t count = 0;
    };

    std::array<GroupRange, 4> groups;

    // Merge two reflection data by combining their bindings and visibilities
    static std::expected<ShaderReflectionData, Error> MergeReflectionData(
        const ShaderReflectionData& a,
        const ShaderReflectionData& b);

    const std::span<const BindingInfo> GetGroup(uint32_t setIdx) const {
        return std::span<const BindingInfo>(bindings.begin() + (groups[setIdx].offset),
                                            groups[setIdx].count);
    };
    const std::span<const BindingInfo> GetAllBindings() const { return bindings; }

    const std::span<const MaterialVariableInfo> GetMaterialVariableInfos() const {
        return materialVariables;
    }
};

class ShaderAsset {
    friend class ShaderSystem;

  public:
    using sa = core::ShaderAssetFormat;
    ShaderAsset() = default;
    ShaderAsset(const ShaderAsset&) = delete;
    ShaderAsset& operator=(const ShaderAsset&) = delete;
    ShaderAsset(ShaderAsset&&) noexcept = default;
    ShaderAsset& operator=(ShaderAsset&&) noexcept = default;
    ~ShaderAsset() = default;

    static ShaderAsset Create(wgpu::ShaderModule shaderModule,
                              sa::ShaderVisibility shaderStage,
                              ShaderReflectionData reflection,
                              std::array<wgpu::BindGroupLayout, 4> bindGroupLayouts);

    const wgpu::ShaderModule& GetVertexModule() const { return m_vertexModule; }
    const wgpu::ShaderModule& GetFragmentModule() const { return m_fragmentModule; }
    const wgpu::ShaderModule& GetComputeModule() const { return m_computeModule; }

    wgpu::BindGroupLayout GetBindGroupLayout(uint32_t setNumber) const {
        return m_bindGroupLayouts[setNumber];
    }

    bool IsValidRenderShader() {
        return m_vertexModule != nullptr && m_fragmentModule != nullptr &&
               m_shaderStage & (sa::ShaderVisibility::Vertex | sa::ShaderVisibility::Fragment);
    }
    const ShaderReflectionData& GetReflection() const { return m_reflection; }

  private:
    ShaderAsset(wgpu::ShaderModule vertex,
                wgpu::ShaderModule fragment,
                wgpu::ShaderModule compute,
                sa::ShaderVisibility shaderStage,
                ShaderReflectionData reflection,
                std::array<wgpu::BindGroupLayout, 4> bindGroupLayout)
        : m_vertexModule(vertex),
          m_fragmentModule(fragment),
          m_computeModule(compute),
          m_shaderStage(shaderStage),
          m_reflection(std::move(reflection)),
          m_bindGroupLayouts(bindGroupLayout) {}

    wgpu::ShaderModule m_vertexModule = nullptr;
    wgpu::ShaderModule m_fragmentModule = nullptr;
    wgpu::ShaderModule m_computeModule = nullptr;
    std::array<wgpu::BindGroupLayout, 4> m_bindGroupLayouts;
    sa::ShaderVisibility m_shaderStage = sa::ShaderVisibility::None;
    ShaderReflectionData m_reflection;
};
}  // namespace core::render
