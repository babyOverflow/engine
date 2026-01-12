#pragma once
#include <tiny_gltf.h>
#include <string_view>

#include "ResourcePool.h"
#include "memory/StridedSpan.h"
#include "render/Mesh.h"
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

    return ReturnT (core::memory::StridedSpan<ElementT>(ptr, stride, accessor.count));
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

class AssetManager {
  public:
    static AssetManager Create(render::Device* device);
    AssetManager() = delete;

    Handle LoadModel(std::string filePath);
    render::Model* GetModel(Handle handle);

    Handle LoadShader(const std::string & shaderPath);
    render::GpuShaderModule* GetShaderModule(Handle handle);

  private:
    std::expected<render::Model, int> LoadModelGLTFInternal(std::string filePath);

    AssetManager(render::Device* device) : m_device(device) {}
    render::Device* m_device;

    ResourcePool<render::Model> m_modelPool;
    // TODO!(m_modelCache)
    ResourcePool<render::GpuShaderModule> m_shaderPool;
    std::unordered_map<std::string, Handle> m_shaderCache;
};
}  // namespace core