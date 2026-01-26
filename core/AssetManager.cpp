
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "AssetManager.h"
#include "ShaderAssetFormat.h"
#include "import/GLTFImporter.h"
#include "render/util.h"

#include <slang.h>

using namespace slang;

namespace core {
AssetManager AssetManager::Create(render::Device* device) {
    return AssetManager(device);
}

Handle AssetManager::StoreModel(render::Model&& model) {
    return m_modelPool.Attach(std::move(model));
}

Handle AssetManager::LoadModel(std::string filePath) {
    auto model = import::GLTFImporter::ImportFromFile(this, m_device, filePath);
    if (!model.has_value()) {
        // TODO! expected error handling
        return Handle();
    }
    return model.value();
}

AssetView<render::Model> core::AssetManager::GetModel(Handle handle) {
    return {m_modelPool.Get(handle), handle};
}

Handle core::AssetManager::LoadShader(const std::string& shaderPath) {
    auto it = m_shaderCache.find(shaderPath);
    if (it != m_shaderCache.end() && it->second.IsValid()) {
        return it->second;
    }

    const auto shrdOrError = util::ReadFileToByte(shaderPath).and_then(ShaderAsset::LoadFromMemory);
    if (!shrdOrError.has_value()) {
        return Handle();
    }
    const auto shdr = shrdOrError.value();

    std::string_view wgslCode(reinterpret_cast<const char*>(shdr.code.data()), shdr.code.size());
    render::GpuShaderModule shader = m_device->CreateShaderModuleFromWGSL(wgslCode);

    util::WgpuShaderBindingLayoutInfo entries = core::util::MapShdrBindToWgpu(shdr.bindings);
    shader.SetBindGroupEntries(entries);

    Handle handle = m_shaderPool.Attach(std::move(shader));
    m_shaderCache[shaderPath] = handle;
    return handle;
}

AssetView<render::GpuShaderModule> core::AssetManager::GetShaderModule(Handle handle) {
    return {m_shaderPool.Get(handle), handle};
}

}  // namespace core