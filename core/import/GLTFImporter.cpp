#include "GLTFImporter.h"

#include "AssetManager.h"
#include "render/Material.h"

namespace core::importer {

const std::string kGltfPosition = "POSITION";
const std::string kGltfTexCoord0 = "TEXCOORD_0";
const std::string kGltfNormal = "NORMAL";
const std::string kGltfTangent = "TANGENT";

std::expected<render::Material, Error> ImportMaterial(AssetManager* assetManger,
                                                      render::MaterialSystem* materialSystem,
                                                      tinygltf::Model& model,
                                                      const tinygltf::Material& gltfMaterial) {
    AssetView<render::ShaderAsset> pbrShader = assetManger->GetStandardPBRShader();
    render::Material material = materialSystem->CreateMaterialFromShader(pbrShader);

    const auto tryAppendTexture = [&](const auto& textureInfo, const std::string& name) {
        if (textureInfo.index < 0) {
            return;
        }
        tinygltf::Texture texture = model.textures[textureInfo.index];
        tinygltf::Image image = model.images[texture.source];
        Handle textureHandle = assetManger->LoadTexture(model, image);


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
}

std::expected<Handle, Error> core::importer::GLTFImporter::ImportFromFile(
    AssetManager* assetManager,
    render::MaterialSystem* materialSystem,
    const render::Device* device,
    const std::string& filePath) {
    using render::Vertex;

    tinygltf::Model gltfModel;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warm;
    bool ret = loader.LoadASCIIFromFile(&gltfModel, &err, &warm, filePath);
    if (!ret) {
        // TODO! log
        return std::unexpected(Error::Parse("Failed to load glTF file: " + err));
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
                .vertexBuffer = device->CreateBufferFromData(
                    packedPositions.data(), packedPositions.size() * sizeof(Vertex),
                    wgpu::BufferUsage::Vertex),
                .indexBuffer =
                    device->CreateBufferFromData(indices, indexSize, wgpu::BufferUsage::Index),
                .indexCount = indexAccessor.count,
                .indexFormat = indexFormat,
            };
            model.subMeshes.emplace_back(std::move(subMesh));
        }
    }
    return assetManager->StoreModel(std::move(model));
}

}  // namespace core::importer