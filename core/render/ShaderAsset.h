#pragma once
#include <dawn/webgpu_cpp.h>
#include <array>
#include <expected>
#include <memory>

#include "Common.h"
#include "ShaderAssetFormat.h"
#include "render.h"

namespace core::render {

struct MaterialVariableInfo {
    PropertyId id;
    size_t offset;
    size_t size;
};

class ShaderReflection {
  public:
    using Binding = ShaderAssetFormat::Binding;
    using Parameter = ShaderAssetFormat::ShaderParameter;

    static ShaderReflection Create(ShaderAssetFormat* shaderAssetFormat);


    size_t materialUniformSize = 0;

    struct GroupRange {
        uint32_t offset = 0;
        uint32_t count = 0;
    };


    std::span<const Binding> GetGroup(uint32_t entryIdx,uint32_t setIdx) const;
    std::span<const Binding> GetAllBindings() const;

    std::span<const MaterialVariableInfo> GetMaterialVariableInfos() const;

    std::optional<uint32_t> GetEntryPointOffsetByName(const std::string& name) const;

    uint32_t GetEntryPointCount() const { return m_shaderAssetFormat->entryPoints.size(); }

    std::span<const ShaderAssetFormat::ShaderParameter> GetEntryInput(uint32_t entryIdx) const;
    std::span<const ShaderAssetFormat::ShaderParameter> GetEntryInputByName(const std::string& name) const;

  private:
    ShaderReflection(ShaderAssetFormat* shaderAssetFormat,
                     std::vector<std::string_view>&& nametable,
                     std::vector<std::array<GroupRange, 4>>&& layouts)
        : m_shaderAssetFormat(shaderAssetFormat), m_nameTable(std::move(nametable)), m_layouts(std::move(layouts)) {}

    const ShaderAssetFormat* m_shaderAssetFormat;
    std::vector<std::string_view> m_nameTable;
    std::vector<std::array<GroupRange, 4>> m_layouts;

    std::string_view GetNameByIndex(uint32_t idx) const;
};

class ShaderAsset {
    friend class ShaderManager;

  public:
    using sa = core::ShaderAssetFormat;
    ShaderAsset() = default;
    ShaderAsset(const ShaderAsset&) = delete;
    ShaderAsset& operator=(const ShaderAsset&) = delete;
    ShaderAsset(ShaderAsset&&) noexcept = default;
    ShaderAsset& operator=(ShaderAsset&&) noexcept = default;
    ~ShaderAsset() = default;

    static ShaderAsset Create(wgpu::ShaderModule shaderModule,
                              std::unique_ptr<ShaderAssetFormat> shaderAssetFormat,
                              ShaderReflection shaderReflection,
                              std::vector<std::array<wgpu::BindGroupLayout, 4>> bindGroupLayoutSets);

    const wgpu::ShaderModule& GetShaderModule() const { return m_shaderModule; }

    wgpu::BindGroupLayout GetBindGroupLayout(uint32_t entry, uint32_t setNumber) const {
        return GetBindGroupLayoutsByEntry(entry)[setNumber];
    }

    std::span<const wgpu::BindGroupLayout> GetBindGroupLayoutsByEntry(uint32_t entry) const {
        return m_bindGroupLayouts[entry];
    }

    const ShaderReflection& GetReflection() const { return m_reflection; }

  private:
    ShaderAsset(wgpu::ShaderModule shaderModule,
                std::unique_ptr<ShaderAssetFormat> shaderAssetFormat,
                ShaderReflection reflection,

                std::vector<std::array<wgpu::BindGroupLayout, 4>> bindGroupLayout)
        : m_shaderModule(shaderModule),
          m_shaderAssetFormat(std::move(shaderAssetFormat)),
          m_reflection(reflection),
          m_bindGroupLayouts(bindGroupLayout) {}

    wgpu::ShaderModule m_shaderModule = nullptr;
    std::vector<std::array<wgpu::BindGroupLayout, 4>> m_bindGroupLayouts;

    std::unique_ptr<ShaderAssetFormat> m_shaderAssetFormat;
    ShaderReflection m_reflection;
};
}  // namespace core::render
