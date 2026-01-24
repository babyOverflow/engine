
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "AssetManager.h"
#include "ShaderAssetFormat.h"
#include "asset/StandardPBR_FS.h"
#include "asset/StandardPBR_VS.h"
#include "render/util.h"

#include <slang.h>

const std::string kGltfPosition = "POSITION";
const std::string kGltfTexCoord0 = "TEXCOORD_0";
const std::string kGltfNormal = "NORMAL";
const std::string kGltfTangent = "TANGENT";

using namespace slang;

namespace core {
AssetManager AssetManager::Create(render::Device* device) {
    return AssetManager(device);
}

AssetManager::AssetManager(render::Device* device) : m_device(device) {
    auto vtxShdrOrError = ShaderAssetFormat::LoadFromMemory(kStandardPBR_VS_Data);
    auto frgShdrOrError = ShaderAssetFormat::LoadFromMemory(kStandardPBR_FS_Data);

    if (!vtxShdrOrError.has_value()) {
        assert(false && "Failed to load standard PBR vertex shader");
    }
    if (!frgShdrOrError.has_value()) {
        assert(false && "Failed to load standard PBR fragment shader");
    }

    render::ShaderAsset vtxShaderAsset;
    {
        ShaderAssetFormat vtxShdr = vtxShdrOrError.value();
        std::string_view wgslCode(reinterpret_cast<const char*>(vtxShdr.code.data()),
                                  vtxShdr.code.size());
        wgpu::ShaderModule shader = m_device->CreateShaderModuleFromWGSL(wgslCode);
        vtxShaderAsset = render::ShaderAsset::CreateShaderAsset(
            shader, vtxShdr.header.entryShaderStage, std::move(vtxShdr.bindings));
    }

    render::ShaderAsset frgShaderAsset;
    {
        ShaderAssetFormat frgShdr = frgShdrOrError.value();
        std::string_view wgslCode(reinterpret_cast<const char*>(frgShdr.code.data()),
                                  frgShdr.code.size());
        wgpu::ShaderModule shader = m_device->CreateShaderModuleFromWGSL(wgslCode);
        frgShaderAsset = render::ShaderAsset::CreateShaderAsset(
            shader, frgShdr.header.entryShaderStage, std::move(frgShdr.bindings));
    }
    auto renderShaderOrError =
        render::ShaderAsset::MergeShaderAsset(vtxShaderAsset, frgShaderAsset);
    if (!renderShaderOrError.has_value()) {
        assert(false && "Failed to merge vtx frag shaders");
    }

    Handle handle = m_shaderPool.Attach(std::move(renderShaderOrError.value()));
    m_shaderCache[std::string(kStandardShader)] = handle;
}

Handle AssetManager::LoadTexture(tinygltf::Model& model, const tinygltf::Image& image) {
    wgpu::TextureDescriptor desc;
    desc.size.width = image.width;
    desc.size.height = image.height;
    desc.size.depthOrArrayLayers = 1;
    desc.mipLevelCount = 1;
    desc.sampleCount = 1;
    desc.dimension = wgpu::TextureDimension::e2D;

    switch (image.pixel_type) {
        case TINYGLTF_COMPONENT_TYPE_BYTE:
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
            desc.format = wgpu::TextureFormat::RGBA8Unorm;
            break;
        case TINYGLTF_COMPONENT_TYPE_SHORT:
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
            desc.format = wgpu::TextureFormat::RGBA16Uint;
            break;
        case TINYGLTF_COMPONENT_TYPE_FLOAT:
            desc.format = wgpu::TextureFormat::RGBA32Float;
            break;
        default:
            desc.format = wgpu::TextureFormat::RGBA8Unorm;
            break;
    }
    desc.usage = wgpu::TextureUsage::TextureBinding | wgpu::TextureUsage::CopyDst;

    core::memory::StridedSpan<const uint8_t> imageData{
        image.image.data(), 1, static_cast<size_t>(image.width * image.height * 4)};
    render::GpuTexture texture = m_device->CreateTextureFromData(desc, imageData);

    texture.SetDesc(desc.usage, desc.dimension, desc.format, desc.size);

    auto handle = m_texturePool.Attach(std::move(texture));

    return handle;
}

Handle AssetManager::LoadMaterial(tinygltf::Model& model, const tinygltf::Material& gltfMaterial) {
    render::Material material = m_materialSystem.CreateMaterialFromShader(
        m_shaderPool.Get(m_shaderCache[std::string(kStandardShader)]));

    const auto tryAppendTexture = [&](const auto& textureInfo, const std::string& name) {
        if (textureInfo.index < 0) {
            return;
        }
        tinygltf::Texture texture = model.textures[textureInfo.index];
        tinygltf::Image image = model.images[texture.source];
        Handle textureHandle = LoadTexture(model, image);
        material.SetTexture(name, textureHandle);
    };

    tryAppendTexture(gltfMaterial.pbrMetallicRoughness.baseColorTexture, "u_BaseColorTexture");
    tryAppendTexture(gltfMaterial.pbrMetallicRoughness.metallicRoughnessTexture,
                     "u_MetallicRoughnessTexture");
    tryAppendTexture(gltfMaterial.normalTexture, "u_NormalTexture");
    tryAppendTexture(gltfMaterial.occlusionTexture, "u_OcclusionTexture");
    tryAppendTexture(gltfMaterial.emissiveTexture, "u_EmissiveTexture");

    material.SetVariable("u_BaseColorFactor",
                         glm::vec4(gltfMaterial.pbrMetallicRoughness.baseColorFactor[0],
                                   gltfMaterial.pbrMetallicRoughness.baseColorFactor[1],
                                   gltfMaterial.pbrMetallicRoughness.baseColorFactor[2],
                                   gltfMaterial.pbrMetallicRoughness.baseColorFactor[3]));
    material.SetVariable("u_EmissiveFactor",
                         glm::vec3(gltfMaterial.emissiveFactor[0], gltfMaterial.emissiveFactor[1],
                                   gltfMaterial.emissiveFactor[2]));
    material.SetVariable("u_NormalScale", gltfMaterial.normalTexture.scale);

    material.SetVariable("u_MetallicFactor", gltfMaterial.pbrMetallicRoughness.metallicFactor);
    material.SetVariable("u_RoughnessFactor", gltfMaterial.pbrMetallicRoughness.roughnessFactor);

    Handle handle = m_materialPool.Attach(std::move(material));
    return handle;
}

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
    using sa = ShaderAssetFormat;

    auto it = m_shaderCache.find(shaderPath);
    if (it != m_shaderCache.end() && it->second.IsValid()) {
        return it->second;
    }

    const auto shrdOrError =
        util::ReadFileToByte(shaderPath).and_then(ShaderAssetFormat::LoadFromMemory);
    if (!shrdOrError.has_value()) {
        return Handle();
    }
    const auto shdr = shrdOrError.value();

    std::string_view wgslCode(reinterpret_cast<const char*>(shdr.code.data()), shdr.code.size());
    wgpu::ShaderModule shader = m_device->CreateShaderModuleFromWGSL(wgslCode);

    render::ShaderAsset shaderAsset =
        render::ShaderAsset::CreateShaderAsset(shader, shdr.header.entryShaderStage, shdr.bindings);

    Handle handle = m_shaderPool.Attach(std::move(shaderAsset));
    m_shaderCache[shaderPath] = handle;
    return handle;
}

render::ShaderAsset* core::AssetManager::GetShaderAsset(Handle handle) {
    return m_shaderPool.Get(handle);
}

}  // namespace core