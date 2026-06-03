#pragma once
#include <bitset>
#include <concepts>
#include <map>
#include <unordered_map>
#include <vector>

#include "LayoutCache.h"
#include "ShaderAsset.h"
#include "Texture.h"
#include "render.h"


namespace core::render {
class PassManager;

template <typename T>
concept ValidMaterialVariableType =
    std::is_same_v<T, float> || std::is_same_v<T, int32_t> || std::is_same_v<T, uint32_t> ||
    std::is_same_v<T, glm::vec2> || std::is_same_v<T, glm::vec3> || std::is_same_v<T, glm::vec4> ||
    std::is_same_v<T, glm::mat4>;

class Material {
    friend class MaterialManager;

  public:
    Material();
    ~Material() = default;

    Material(const Material&) = delete;
    Material& operator=(const Material&) = delete;

    Material(Material&& rhs) noexcept;
    Material& operator=(Material&& rhs) noexcept;

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
    }

    template <ValidMaterialVariableType T>
    void SetVariable(const std::string& name, const T value) {
        SetVariable(ToPropertyID(name), value);
    }

    bool IsDirty() const;

    AssetView<ShaderAsset> GetShader() const { return m_shaderView; }

    std::span<const uint8_t> GetActivePassIDs() const { return m_activePassIds; }
    void AddPassID(uint8_t passId) { m_activePassIds.push_back(passId); }

    void UpdateUniform();

    wgpu::BindGroup GetBindGroup(uint32_t passId) const;

    void FlushDirtyBindGroups();

  private:
    struct VariableInfo {
        size_t offset;
        size_t size;
    };

    Material(Device* device,
             PassManager* passManager,
             AssetView<ShaderAsset> shaderView,
             wgpu::Buffer uniformBuffer = nullptr,
             std::vector<std::byte> cpuData = {},
             std::unordered_map<PropertyId, VariableInfo> variableInfo = {});

    wgpu::BindGroup CreateBindGroupForPass(uint32_t passId);
    void RebuildBindGroup();

    Device* m_device = nullptr;
    PassManager* m_passManager = nullptr;
    AssetView<ShaderAsset> m_shaderView;
    std::array<wgpu::BindGroup, 255> m_bindGroups;
    std::bitset<255> m_bindGroupDirtyFlags;
    wgpu::Sampler m_sampler = nullptr;

    wgpu::Buffer m_uniformBuffer = nullptr;
    std::vector<std::byte> m_cpuVariableBufferData;
    std::unordered_map<PropertyId, VariableInfo> m_variableInfo;

    std::unordered_map<PropertyId, AssetView<Texture>> m_textures;
    std::vector<uint8_t> m_activePassIds;
};

}  // namespace core::render
