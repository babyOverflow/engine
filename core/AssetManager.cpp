
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "AssetManager.h"
#include "ShaderAssetFormat.h"
#include "render/util.h"

#include <slang.h>

using namespace slang;

namespace core {
AssetManager AssetManager::Create(render::Device* device) {
    return AssetManager(device);
}

const std::string kGltfPosition = "POSITION";
const std::string kGltfTexCoord0 = "TEXCOORD_0";
const std::string kGltfNormal = "NORMAL";
const std::string kGltfTangent = "TANGENT";

std::expected<render::Model, int> core::AssetManager::LoadModelGLTFInternal(std::string filePath) {
    using render::Vertex;

    tinygltf::Model gltfModel;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warm;

    bool ret = loader.LoadASCIIFromFile(&gltfModel, &err, &warm, filePath);
    if (!ret) {
        // TODO! log
        return std::unexpected(-1);
    }
    render::Model model;
    for (const auto& mesh : gltfModel.meshes) {
        for (const auto& primitive : mesh.primitives) {
            if (primitive.attributes.find(kGltfTexCoord0) == primitive.attributes.end()) {
                continue;
            }
            if (primitive.indices < 0) {
                continue;
            }
            const auto posResult = GetAttributePtr<glm::vec3>(gltfModel, primitive, kGltfPosition);
            const auto texResult =
                GetAttributePtr<const glm::vec2>(gltfModel, primitive, kGltfTexCoord0);
            const auto norResult =
                GetAttributePtr<const glm::vec3>(gltfModel, primitive, kGltfNormal);
            const auto tanResult =
                GetAttributePtr<const glm::vec4>(gltfModel, primitive, kGltfTangent);
            if (!posResult.has_value() || !texResult.has_value()) {
                continue;
            }
            auto posSpan = posResult.value();
            auto texSpan = texResult.value();
            auto norSpan =
                norResult.has_value()
                    ? norResult.value()
                    : core::memory::StridedSpan<const glm::vec3>::MakeZero(posSpan.size());
            auto tanSpan =
                tanResult.has_value()
                    ? tanResult.value()
                    : core::memory::StridedSpan<const glm::vec4>::MakeZero(posSpan.size());

            std::vector<Vertex> packedPositions;

            packedPositions.reserve(posSpan.size());
            for (size_t i = 0; i < posSpan.size(); ++i) {
                Vertex v{.position = posSpan[i],
                         .normal = norSpan[i],
                         .uv = texSpan[i],
                         .tangent = tanSpan[i]};
                packedPositions.push_back(v);
            }

            const auto& indexAccessor = gltfModel.accessors[primitive.indices];
            const auto& indexBufferView = gltfModel.bufferViews[indexAccessor.bufferView];
            const auto& indexBuffer = gltfModel.buffers[indexBufferView.buffer];

            const void* indices =
                &indexBuffer.data[indexBufferView.byteOffset + indexAccessor.byteOffset];
            size_t indexSize = tinygltf::GetComponentSizeInBytes(indexAccessor.componentType) *
                               indexAccessor.count;
            wgpu::IndexFormat indexFormat = wgpu::IndexFormat::Undefined;
            if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
                indexFormat = wgpu::IndexFormat::Uint32;
            } else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
                indexFormat = wgpu::IndexFormat::Uint16;
            } else {
                continue;
            }

            render::SubMesh subMesh{
                .vertexBuffer = m_device->CreateBufferFromData(
                    packedPositions.data(), packedPositions.size() * sizeof(Vertex),
                    wgpu::BufferUsage::Vertex),
                .indexBuffer =
                    m_device->CreateBufferFromData(indices, indexSize, wgpu::BufferUsage::Index),
                .indexCount = indexAccessor.count,
                .indexFormat = indexFormat,
            };
            model.subMeshes.emplace_back(std::move(subMesh));
        }
    }
    return model;
}

Handle AssetManager::LoadModel(std::string filePath) {
    auto model = LoadModelGLTFInternal(filePath);
    if (!model.has_value()) {
        // TODO! err
        return Handle();
    }
    auto m = std::move(*model);
    return m_modelPool.Attach(std::move(m));
}

render::Model* AssetManager::GetModel(Handle handle) {
    return m_modelPool.Get(handle);
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

    std::string_view wgslCode(reinterpret_cast<const char*> (shdr.code.data()), shdr.code.size());
    render::GpuShaderModule shader = m_device->CreateShaderModuleFromWGSL(wgslCode);

     util::WgpuShaderBindingLayoutInfo entries =
        core::util::MapShdrBindToWgpu(shdr.bindings);
    shader.SetBindGroupEntries(entries);

    Handle handle = m_shaderPool.Attach(std::move(shader));
    m_shaderCache[shaderPath] = handle;
    return handle;
}

render::GpuShaderModule* AssetManager::GetShaderModule(Handle handle) {
    return m_shaderPool.Get(handle);
}

}  // namespace core