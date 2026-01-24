#pragma once
#include <map>

#include "LayoutCache.h"
#include "PipelineManager.h"
#include "ShaderAsset.h"
#include "render.h"

namespace core::render {

class Material {
  public:
    Material() = default;
    ~Material() = default;

    Material(const Material&) = delete;
    Material& operator=(const Material&) = delete;

    Material(Material&&) noexcept = default;
    Material& operator=(Material&&) noexcept = default;

    Material(wgpu::BindGroupLayout bindGroupLayout)
        : m_bindGroupLayout(bindGroupLayout) {}


    void SetShader(ShaderAsset* shaderModule);

    void SetTexture(const std::string& name, Handle textureHandle);
    void SetVariable(const std::string& name, float value);
    void SetVariable(const std::string& name, const glm::vec3& value);
    void SetVariable(const std::string& name, const glm::vec4& value);

  private:
    ShaderAsset* m_shaderAsset = nullptr;
    wgpu::BindGroupLayout m_bindGroupLayout;
    wgpu::TextureView m_textureView;
    GpuBindGroup* m_bindGroup = nullptr;

    std::map<std::string, Handle> m_textureHandles;

    struct VariableInfo {
        size_t offset;
        size_t size;
    };
    std::map<std::string, size_t> m_variableInfo;
    std::vector<std::byte> m_variableBufferData;

};

class MaterialSystem {
  public:
    MaterialSystem() = delete;
    MaterialSystem(Device* device,
                            LayoutCache* layoutCache,
                            PipelineManager* piplineManager);
    ~MaterialSystem() = default;
    MaterialSystem(const MaterialSystem&) = delete;
    MaterialSystem& operator=(const MaterialSystem&) = delete;
    MaterialSystem(MaterialSystem&&) noexcept = default;
    MaterialSystem& operator=(MaterialSystem&&) noexcept = default;

    void RegisterShader(ShaderAsset* shaderModule);

    Material CreateMaterialFromShader(const ShaderAsset* shaderAsset);

  private:
    Device* m_device;
    LayoutCache* m_layoutCache;
    PipelineManager* m_pipelineManager;
};

}  // namespace core::render