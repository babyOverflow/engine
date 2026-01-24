#pragma once
#include <dawn/webgpu_cpp.h>
#include <expected>

#include "Common.h"
#include "ShaderAssetFormat.h"

namespace core::render {

class ShaderAsset {
  public:
    using sa = core::ShaderAssetFormat;
    ShaderAsset() = default;
    ShaderAsset(const ShaderAsset&) = delete;
    ShaderAsset& operator=(const ShaderAsset&) = delete;
    ShaderAsset(ShaderAsset&&) = default;
    ShaderAsset& operator=(ShaderAsset&&) = default;
    ~ShaderAsset() = default;

    static ShaderAsset CreateShaderAsset(wgpu::ShaderModule shaderModule,
                                         sa::ShaderVisibility shaderStage,
                                         const std::vector<sa::Binding> bindings);

    static std::expected<ShaderAsset, Error> MergeShaderAsset(const ShaderAsset& a,
                                                              const ShaderAsset& b);

    const wgpu::ShaderModule& GetVertexModule() const { return m_vertexModule; }
    const wgpu::ShaderModule& GetFragmentModule() const { return m_fragmentModule; }
    const wgpu::ShaderModule& GetComputeModule() const { return m_computeModule; }

    bool IsVaildRenderShader() {
        return m_vertexModule != nullptr && m_fragmentModule != nullptr &&
               m_shaderStage & (sa::ShaderVisibility::Vertex | sa::ShaderVisibility::Fragment);
    }
    const std::vector<sa::Binding>& GetBindings() const { return m_bindings; }

  private:
    ShaderAsset(wgpu::ShaderModule vertex,
                wgpu::ShaderModule fragment,
                wgpu::ShaderModule compute,
                sa::ShaderVisibility shaderStage,
                std::vector<sa::Binding> bindings)
        : m_vertexModule(vertex),
          m_fragmentModule(fragment),
          m_computeModule(compute),
          m_shaderStage(shaderStage),
          m_bindings(std::move(bindings)) {}

    wgpu::ShaderModule m_vertexModule = nullptr;
    wgpu::ShaderModule m_fragmentModule = nullptr;
    wgpu::ShaderModule m_computeModule = nullptr;
    sa::ShaderVisibility m_shaderStage = sa::ShaderVisibility::None;
    std::vector<sa::Binding> m_bindings;
};
}  // namespace core::render
