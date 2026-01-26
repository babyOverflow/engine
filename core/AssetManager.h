#pragma once
#include <tiny_gltf.h>
#include <string_view>

#include "ResourcePool.h"
#include "memory/StridedSpan.h"
#include "render/Mesh.h"
#include "render/ShaderAsset.h"
#include "render/Material.h"

#include "util/Load.h"

namespace core {

template <typename T, typename ModuleT>
static auto GetAttributePtrImpl(ModuleT& model,
                                const tinygltf::Primitive& primitive,
                                const std::string& name)
    -> std::expected<core::memory::StridedSpan<T>, int> {
    using ElementT = std::conditional_t<std::is_const_v<ModuleT>, const T, T>;
    using ReturnT = std::expected<core::memory::StridedSpan<ElementT>, int>;

    if (primitive.attributes.find(name) == primitive.attributes.end()) {
        return std::unexpected(-1);
    }
    const auto& accessor = model.accessors[primitive.attributes.at(name)];
    const auto& bufferView = model.bufferViews[accessor.bufferView];
    auto& buffer = model.buffers[bufferView.buffer];

    int stride = accessor.ByteStride(bufferView);
    auto* ptr = reinterpret_cast<core::memory::StridedSpan<T>::InternalPtr>(
        buffer.data.data() + bufferView.byteOffset + accessor.byteOffset);

    return ReturnT(core::memory::StridedSpan<ElementT>(ptr, stride, accessor.count));
}

template <typename T>
static auto GetAttributePtr(const tinygltf::Model& model,
                            const tinygltf::Primitive& primitive,
                            const std::string& name)
    -> std::expected<core::memory::StridedSpan<const T>, int> {
    return GetAttributePtrImpl<T>(model, primitive, name);
}

template <typename T>
static auto GetAttributePtr(tinygltf::Model& model,
                            const tinygltf::Primitive& primitive,
                            const std::string& name)
    -> std::expected<core::memory::StridedSpan<T>, int> {
    return GetAttributePtrImpl<T>(model, primitive, name);
}

constexpr std::string_view kStandardShader("StandardPBR");

class AssetManager {
  public:
    static AssetManager Create(render::Device* device);
    AssetManager() = delete;

    Handle LoadTexture(tinygltf::Model& model, const tinygltf::Image& image);

    Handle LoadMaterial(tinygltf::Model& model, const tinygltf::Material& material);

    Handle LoadModel(std::string filePath);
    Handle StoreModel(render::Model&& model);
    AssetView<render::Model> GetModel(Handle handle);

    Handle LoadShader(const std::string & shaderPath);
    AssetView<render::ShaderAsset> GetShaderAsset(Handle handle);

  private:
    AssetManager(render::Device* device) : m_device(device) {}
    render::Device* m_device;
    render::MaterialSystem m_materialSystem;

    ResourcePool<render::ShaderAsset> m_shaderPool;
    ResourcePool<render::GpuTexture> m_texturePool;
    ResourcePool<render::Material> m_materialPool;
    // TODO!(m_modelCache)
    ResourcePool<render::Model> m_modelPool;
    std::unordered_map<std::string,
                       Handle,
                       std::hash<std::string_view>,  
                       std::equal_to<>>
        m_shaderCache;


};
}  // namespace core