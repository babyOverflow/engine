#pragma once
#include <concepts>
#include <map>

#include "LayoutCache.h"
#include "PipelineManager.h"
#include "ShaderAsset.h"
#include "Texture.h"
#include "render.h"

namespace core::render {

template <typename T>
concept ValidMaterialVariableType =
    std::is_same_v<T, float> || std::is_same_v<T, int32_t> || std::is_same_v<T, uint32_t> ||
    std::is_same_v<T, glm::vec2> || std::is_same_v<T, glm::vec3> || std::is_same_v<T, glm::vec4> ||
    std::is_same_v<T, glm::mat4>;

class Material {
    friend class MaterialManager;

  public:
    Material() = default;
    ~Material() = default;

    Material(const Material&) = delete;
    Material& operator=(const Material&) = delete;

    Material(Material&&) noexcept = default;
    Material& operator=(Material&&) noexcept = default;

    void SetTexture(const std::string& name, AssetView<Texture> texture);
    void SetTexture(PropertyId id, AssetView<Texture> texture);

    template <ValidMaterialVariableType T>
    void SetVariable(PropertyId id, const T value) {
        auto it = m_variableInfo.find(id);
        if (it == m_variableInfo.end()) {
            return;
        }
        size_t offset = it->second.offset;
        std::memcpy(m_cpuVariableBufferData.data() + offset, &value, sizeof(T));
        m_isUniformDirty = true;
    }

    template <ValidMaterialVariableType T>
    void SetVariable(const std::string& name, const T value) {
        SetVariable(ToPropertyID(name), value);
    }

    AssetView<ShaderAsset> GetShader() const { return m_shaderView; }

    void UpdateUniform();

    wgpu::BindGroup GetBindGroup();

  private:
    struct VariableInfo {
        size_t offset;
        size_t size;
    };

    Material(Device* device,
             AssetView<ShaderAsset> shaderView,
             wgpu::Buffer unifromBuffer = nullptr,
             std::vector<std::byte> cpuData = {},
             std::unordered_map<PropertyId, VariableInfo> variableInfo = {});

    void RebuildBindGroup();

    bool m_isUniformDirty = true;
    Device* m_device;
    wgpu::BindGroup m_bindGroup = nullptr;
    AssetView<ShaderAsset> m_shaderView;

    wgpu::Buffer m_uniformBuffer;
    std::vector<std::byte> m_cpuVariableBufferData;
    std::unordered_map<PropertyId, VariableInfo> m_variableInfo;

    std::unordered_map<PropertyId, AssetView<Texture>> m_textures;
    wgpu::Sampler m_sampler = nullptr;  // TODO: support sampler in material
};

}  // namespace core::render
