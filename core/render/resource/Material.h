#pragma once
#include <bitset>
#include <concepts>
#include <map>
#include <unordered_map>
#include <vector>

#include "Texture.h"
#include "render/backend/BindGroupFactory.h"
#include "render/backend/LayoutCache.h"
#include "render/render.h"

namespace core::render {
class PassManager;

template <typename T>
concept ValidMaterialVariableType =
    std::is_same_v<T, float> || std::is_same_v<T, int32_t> || std::is_same_v<T, uint32_t> ||
    std::is_same_v<T, glm::vec2> || std::is_same_v<T, glm::vec3> || std::is_same_v<T, glm::vec4> ||
    std::is_same_v<T, glm::mat4>;

class Material {
    friend class MaterialMutator;
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

    AssetView<Texture> GetTexture(PropertyId id);

    wgpu::Sampler GetSampler() { return m_sampler; }

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

    uint8_t GetActiveTechniqueID() const { return m_activeTechniqueId; }
    void SetTechniqueID(uint8_t techniqueId) { m_activeTechniqueId = techniqueId; }

    void UpdateUniform();

  private:
    struct VariableInfo {
        size_t offset;
        size_t size;
    };

    Material(Device* device,
             wgpu::Buffer uniformBuffer = nullptr,
             std::vector<std::byte> cpuData = {},
             std::unordered_map<PropertyId, VariableInfo> variableInfo = {});

    // void RebuildBindGroup();

    Device* m_device = nullptr;
    bool m_isDirty = true;

    wgpu::Sampler m_sampler = nullptr;
    wgpu::Buffer m_uniformBuffer = nullptr;
    std::vector<std::byte> m_cpuVariableBufferData;
    std::unordered_map<PropertyId, VariableInfo> m_variableInfo;

    std::unordered_map<PropertyId, AssetView<Texture>> m_textures;

    uint8_t m_activeTechniqueId;
};

struct MaterialProvider {
    AssetView<Material> material;
    wgpu::TextureView GetTextureView(PropertyId id);
};
static_assert(BindGroupResourceProvider<MaterialProvider>);

}  // namespace core::render
