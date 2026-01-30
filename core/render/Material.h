#pragma once
#include <concepts>
#include <map>

#include "LayoutCache.h"
#include "PipelineManager.h"
#include "ShaderAsset.h"
#include "render.h"

namespace core::render {

template <typename T>
concept ValidMaterialVariableType =
    std::is_same_v<T, float> || std::is_same_v<T, int32_t> || std::is_same_v<T, uint32_t> ||
    std::is_same_v<T, glm::vec2> || std::is_same_v<T, glm::vec3> || std::is_same_v<T, glm::vec4> ||
    std::is_same_v<T, glm::mat4>;

class Material {
    friend class MaterialSystem;

  public:
    Material() = default;
    ~Material() = default;

    Material(const Material&) = delete;
    Material& operator=(const Material&) = delete;

    Material(Material&&) noexcept = default;
    Material& operator=(Material&&) noexcept = default;

    void SetTexture(const std::string& name, AssetView<GpuTexture> texture);
    void SetTexture(PropertyId id, AssetView<GpuTexture> texture);

    template <ValidMaterialVariableType T>
    void SetVariable(PropertyId id, const T value) {
        auto it = m_variableInfo.find(id);
        if (it == m_variableInfo.end()) {
            return;
        }
        size_t offset = it->second.offset;
        std::memcpy(m_cpuVariableBufferData.data() + offset, &value, sizeof(T));
    }

    template <ValidMaterialVariableType T>
    void SetVariable(const std::string& name, const T value) {
        SetVariable(ToPropertyID(name), value);
    }

    void UpdateUniform();

    std::expected<wgpu::BindGroup, Error> GetBindGroup();

  private:
    struct VariableInfo {
        size_t offset;
        size_t size;
    };

    Material(Device* device,
             AssetView<ShaderAsset> shaderView,
             wgpu::Buffer unifromBuffer = nullptr,
             std::vector<std::byte> cpuData = {},
             std::unordered_map<PropertyId, VariableInfo> variableInfo = {})
        : m_device(device),
          m_shaderView(shaderView),
          m_uniformBuffer(unifromBuffer),
          m_cpuVariableBufferData(cpuData),
          m_variableInfo(variableInfo) {}

    void RebuildBindGroup();

    bool m_isDirty = true;
    Device* m_device;
    wgpu::BindGroup m_bindGroup = nullptr;
    AssetView<ShaderAsset> m_shaderView;

    wgpu::Buffer m_uniformBuffer;
    std::vector<std::byte> m_cpuVariableBufferData;
    std::unordered_map<PropertyId, VariableInfo> m_variableInfo;

    std::unordered_map<PropertyId, AssetView<GpuTexture>> m_textures;
};

}  // namespace core::render
