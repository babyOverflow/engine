#include "GLTFImporter.h"


namespace core::importer {

const std::string kGltfPosition = "POSITION";
const std::string kGltfTexCoord0 = "TEXCOORD_0";
const std::string kGltfNormal = "NORMAL";
const std::string kGltfTangent = "TANGENT";

//std::expected<render::Material, Error> ImportMaterial(AssetManager* assetManger,
//                                                      tinygltf::Model& model,
//                                                      const tinygltf::Material& gltfMaterial) {
//    const auto tryAppendTexture = [&](const auto& textureInfo, const std::string& name) {
//        if (textureInfo.index < 0) {
//            return;
//        }
//        tinygltf::Texture texture = model.textures[textureInfo.index];
//        tinygltf::Image image = model.images[texture.source];
//        Handle textureHandle = assetManger->LoadTexture(model, image);
//
//        material.SetTexture(name, textureHandle);
//    };
//
//    tryAppendTexture(gltfMaterial.pbrMetallicRoughness.baseColorTexture, "u_BaseColorTexture");
//    tryAppendTexture(gltfMaterial.pbrMetallicRoughness.metallicRoughnessTexture,
//                     "u_MetallicRoughnessTexture");
//    tryAppendTexture(gltfMaterial.normalTexture, "u_NormalTexture");
//    tryAppendTexture(gltfMaterial.occlusionTexture, "u_OcclusionTexture");
//    tryAppendTexture(gltfMaterial.emissiveTexture, "u_EmissiveTexture");
//
//    material.SetVariable("u_BaseColorFactor",
//                         glm::vec4(gltfMaterial.pbrMetallicRoughness.baseColorFactor[0],
//                                   gltfMaterial.pbrMetallicRoughness.baseColorFactor[1],
//                                   gltfMaterial.pbrMetallicRoughness.baseColorFactor[2],
//                                   gltfMaterial.pbrMetallicRoughness.baseColorFactor[3]));
//    material.SetVariable("u_EmissiveFactor",
//                         glm::vec3(gltfMaterial.emissiveFactor[0], gltfMaterial.emissiveFactor[1],
//                                   gltfMaterial.emissiveFactor[2]));
//    material.SetVariable("u_NormalScale", gltfMaterial.normalTexture.scale);
//
//    material.SetVariable("u_MetallicFactor", gltfMaterial.pbrMetallicRoughness.metallicFactor);
//    material.SetVariable("u_RoughnessFactor", gltfMaterial.pbrMetallicRoughness.roughnessFactor);
//}
//
//std::expected<ModelBlob, Error> core::importer::GLTFImporter::ImportFromFile(
//    AssetManager* assetManager,
//    const render::Device* device,
//    const std::string& filePath) {
//    using render::Vertex;
//
//    tinygltf::Model gltfModel;
//    tinygltf::TinyGLTF loader;
//    std::string err;
//    std::string warm;
//    bool ret = loader.LoadASCIIFromFile(&gltfModel, &err, &warm, filePath);
//    if (!ret) {
//        // TODO! log
//        return std::unexpected(Error::Parse("Failed to load glTF file: " + err));
//    }
//    render::Model model;
//    for (const auto& mesh : gltfModel.meshes) {
//        for (const auto& primitive : mesh.primitives) {
//            if (primitive.attributes.find(kGltfTexCoord0) == primitive.attributes.end()) {
//                continue;
//            }
//            if (primitive.indices < 0) {
//                continue;
//            }
//            const auto posResult = GetAttributePtr<glm::vec3>(gltfModel, primitive, kGltfPosition);
//            const auto texResult =
//                GetAttributePtr<const glm::vec2>(gltfModel, primitive, kGltfTexCoord0);
//            const auto norResult =
//                GetAttributePtr<const glm::vec3>(gltfModel, primitive, kGltfNormal);
//            const auto tanResult =
//                GetAttributePtr<const glm::vec4>(gltfModel, primitive, kGltfTangent);
//            if (!posResult.has_value() || !texResult.has_value()) {
//                continue;
//            }
//            auto posSpan = posResult.value();
//            auto texSpan = texResult.value();
//            auto norSpan =
//                norResult.has_value()
//                    ? norResult.value()
//                    : core::memory::StridedSpan<const glm::vec3>::MakeZero(posSpan.size());
//            auto tanSpan =
//                tanResult.has_value()
//                    ? tanResult.value()
//                    : core::memory::StridedSpan<const glm::vec4>::MakeZero(posSpan.size());
//
//            std::vector<Vertex> packedPositions;
//
//            packedPositions.reserve(posSpan.size());
//            for (size_t i = 0; i < posSpan.size(); ++i) {
//                Vertex v{.position = posSpan[i],
//                         .normal = norSpan[i],
//                         .uv = texSpan[i],
//                         .tangent = tanSpan[i]};
//                packedPositions.push_back(v);
//            }
//
//            const auto& indexAccessor = gltfModel.accessors[primitive.indices];
//            const auto& indexBufferView = gltfModel.bufferViews[indexAccessor.bufferView];
//            const auto& indexBuffer = gltfModel.buffers[indexBufferView.buffer];
//
//            const void* indices =
//                &indexBuffer.data[indexBufferView.byteOffset + indexAccessor.byteOffset];
//            size_t indexSize = tinygltf::GetComponentSizeInBytes(indexAccessor.componentType) *
//                               indexAccessor.count;
//            wgpu::IndexFormat indexFormat = wgpu::IndexFormat::Undefined;
//            if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
//                indexFormat = wgpu::IndexFormat::Uint32;
//            } else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
//                indexFormat = wgpu::IndexFormat::Uint16;
//            } else {
//                continue;
//            }
//
//            render::SubMesh subMesh{
//                .vertexBuffer = device->CreateBufferFromData(
//                    packedPositions.data(), packedPositions.size() * sizeof(Vertex),
//                    wgpu::BufferUsage::Vertex),
//                .indexBuffer =
//                    device->CreateBufferFromData(indices, indexSize, wgpu::BufferUsage::Index),
//                .indexCount = indexAccessor.count,
//                .indexFormat = indexFormat,
//            };
//            model.subMeshes.emplace_back(std::move(subMesh));
//        }
//    }
//}

static std::expected<std::vector<uint8_t>, Error> ToRGBA(const std::vector<uint8_t>& src,
                                                         size_t count,
                                                         int components) {
    if (components == 4) {
        return src;
    } else if (components == 3) {
        std::vector<uint8_t> dst(count * 4);
        for (size_t i = 0; i < count; ++i) {
            size_t dstIdx = i * 4;
            size_t srcIdx = i * 3;
            dst[dstIdx] = src[srcIdx];
            dst[dstIdx + 1] = src[srcIdx + 1];
            dst[dstIdx + 2] = src[srcIdx + 2];
            dst[dstIdx + 3] = 255;
        }
        return dst;
    } else {
        // TODO!
        return std::unexpected(Error{ErrorType::AssetParsingError,
                                     "Unsupported format texture components must be 4 or 3!"});
    }
}

std::expected<TextureAssetFormat, Error> ImportTextureFromTinygltf(
    const tinygltf::Model& model,
    const tinygltf::Image& gltfImage) {
    uint32_t width = static_cast<uint32_t>(gltfImage.width);
    uint32_t height = static_cast<uint32_t>(gltfImage.height);

    auto pixelOrError = ToRGBA(gltfImage.image, width * height, gltfImage.component);
    if (!pixelOrError.has_value()) {
        return std::unexpected(pixelOrError.error());
    }
    return TextureAssetFormat{
        .width = width,
        .height = height,
        .channel = 4,
        .format = TextureFormat::RGBA8Unorm,
        .pixelData = std::move(pixelOrError).value(),
    };
}

}  // namespace core::importer