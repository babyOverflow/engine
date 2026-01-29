
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "AssetManager.h"
#include "ShaderAssetFormat.h"
#include "asset/StandardPBR_FS.h"
#include "asset/StandardPBR_VS.h"
#include "import/GLTFImporter.h"
#include "render/ShaderSystem.h"
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
    auto vtxBlobOrError =
        ShaderAssetFormat::LoadFromMemory(kStandardPBR_VS_Data).and_then([&](const auto& shdr) {
            return core::importer::ShdrImporter::ShdrImport(m_device, shdr);
        });
    auto frgBlobOrError =
        ShaderAssetFormat::LoadFromMemory(kStandardPBR_FS_Data).and_then([&](const auto& shdr) {
            return core::importer::ShdrImporter::ShdrImport(m_device, shdr);
        });

    if (!vtxBlobOrError.has_value()) {
        assert(false && "Failed to load standard PBR vertex shader");
    }
    if (!frgBlobOrError.has_value()) {
        assert(false && "Failed to load standard PBR fragment shader");
    }

    core::importer::ShaderBlob& vtxBlob = vtxBlobOrError.value();
    core::importer::ShaderBlob& frgBlob = frgBlobOrError.value();

    // ShaderSystem's layout caching logic relies on reflection data. Merging reflections reduces
    // unused layout cache entries.
    auto reflectionOrError =
        render::ShaderReflectionData::MergeReflectionData(vtxBlob.reflection, frgBlob.reflection);
    if (!reflectionOrError.has_value()) {
        assert(false && "Failed to load standard PBR shader");
    }
    vtxBlob.reflection = reflectionOrError.value();
    frgBlob.reflection = reflectionOrError.value();

    render::ShaderAsset vtxShader = m_shaderSystem->CreateFromShaderSource(vtxBlob);
    render::ShaderAsset frgShader = m_shaderSystem->CreateFromShaderSource(frgBlob);

    auto renderShaderOrError = m_shaderSystem->MergeShaderAsset(vtxShader, frgShader);

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
    AssetView<render::ShaderAsset> shaderAsset =
        GetShaderAsset(m_shaderCache[std::string(kStandardShader)]);
    render::Material material = m_materialSystem->CreateMaterialFromShader(shaderAsset);

    const auto tryAppendTexture = [&](const auto& textureInfo, const std::string& name) {
        if (textureInfo.index < 0) {
            return;
        }
        tinygltf::Texture texture = model.textures[textureInfo.index];
        tinygltf::Image image = model.images[texture.source];
        Handle textureHandle = LoadTexture(model, image);

        AssetView tex = GetTexture(textureHandle);

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

Handle AssetManager::LoadModel(std::string filePath) {
    auto model = importer::GLTFImporter::ImportFromFile(this, m_device, filePath);
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
    if (m_shaderCache.find(shaderPath) != m_shaderCache.end()) {
        return m_shaderCache.at(shaderPath);
    }
    auto shaderBlobOrError = importer::ShdrImporter::ShdrImport(m_device, shaderPath);
    if (!shaderBlobOrError.has_value()) {
        return Handle();
    }

    render::ShaderAsset shaderAsset =
        m_shaderSystem->CreateFromShaderSource(shaderBlobOrError.value());

    Handle handle = m_shaderPool.Attach(std::move(shaderAsset));
    m_shaderCache[shaderPath] = handle;
    return handle;
}

AssetView<render::ShaderAsset> core::AssetManager::GetShaderAsset(Handle handle) {
    return {m_shaderPool.Get(handle), handle};
}

}  // namespace core