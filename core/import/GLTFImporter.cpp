#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <ranges>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "GLTFImporter.h"

namespace core::importer {

const std::string kGltfPosition = "POSITION";
const std::string kGltfTexCoord0 = "TEXCOORD_0";
const std::string kGltfNormal = "NORMAL";
const std::string kGltfTangent = "TANGENT";

AssetPath GLTFImporter::ToTextureID(int gltfTextureIndex) {
    return AssetPath{std::format("virtual://tex/{}", gltfTextureIndex)};
}

AssetPath GLTFImporter::ToMeshId(int gltfMeshIndex) {
    return AssetPath{std::format("virtual://mesh/{}", gltfMeshIndex)};
}

glm::mat4 GetNodeMatrix(const tinygltf::Node& node) {
    glm::mat4 matrix(1.0f);
    if (node.matrix.size() == 16) {
        for (int i = 0; i < 16; ++i) {
            glm::value_ptr(matrix)[i] = static_cast<float>(node.matrix[i]);
        }
    } else {
        glm::vec3 translation(0.0f);
        if (node.translation.size() == 3) {
            translation = glm::vec3(static_cast<float>(node.translation[0]),
                                    static_cast<float>(node.translation[1]),
                                    static_cast<float>(node.translation[2]));
        }
        glm::quat rotation(1.0f, 0.0f, 0.0f, 0.0f);
        if (node.rotation.size() == 4) {
            rotation = glm::quat(static_cast<float>(node.rotation[3]),  // w
                                 static_cast<float>(node.rotation[0]),  // x
                                 static_cast<float>(node.rotation[1]),  // y
                                 static_cast<float>(node.rotation[2])   // z
            );
        }
        glm::vec3 scale(1.0f);
        if (node.scale.size() == 3) {
            scale = glm::vec3(static_cast<float>(node.scale[0]), static_cast<float>(node.scale[1]),
                              static_cast<float>(node.scale[2]));
        }
        matrix = glm::translate(glm::mat4(1.0f), translation) * glm::mat4_cast(rotation) *
                 glm::scale(glm::mat4(1.0f), scale);
    }
    return matrix;
}

std::expected<GLTFImportResult, Error> GLTFImporter::ImportFromFile(const std::string& filePath) {
    GLTFImportResult result;
    tinygltf::Model gltfModel;
    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warm;
    bool ret = loader.LoadASCIIFromFile(&gltfModel, &err, &warm, filePath);
    if (!ret) {
        // TODO! log
        return std::unexpected(Error::Parse("Failed to load glTF file: " + err));
    }

    for (int i = 0; i < gltfModel.textures.size(); ++i) {
        const auto& texture = gltfModel.textures[i];
        const auto& image = gltfModel.images[texture.source];
        auto texAsset = ImportTextureFromTinygltf(gltfModel, image).value_or({});
        result.textures.push_back(TextureResult{texAsset, ToTextureID(i)});
    }

    for (const auto& [idx, gltfMesh] : gltfModel.meshes | std::views::enumerate) {
        auto meshAsset = ImportMesh(gltfModel, gltfMesh).value_or({});
        result.meshes.push_back(MeshResult{meshAsset, ToMeshId(idx)});
    }

    for (const auto& node : gltfModel.nodes) {
        if (node.mesh < 0) {
            continue;
        }

        ModelAssetFormat::Node modelNode;
        modelNode.name = node.name;
        modelNode.meshId = ToMeshId(node.mesh);
        modelNode.localMatrix = GetNodeMatrix(node);

        const auto& gltfMesh = gltfModel.meshes[node.mesh];
        for (const auto& primitive : gltfMesh.primitives) {
            // TODO! material
        }

        result.modelAsset.nodes.push_back(modelNode);
    }

    return result;
}

std::expected<MeshAssetFormat, Error> GLTFImporter::ImportMesh(const tinygltf::Model& gltfModel,
                                                               const tinygltf::Mesh& mesh) {
    size_t totalVertexCount = 0;
    size_t totalIndexCount = 0;
    std::vector<MeshAssetFormat::SubMeshInfo> subMeshInfos;
    subMeshInfos.reserve(mesh.primitives.size());
    for (const auto& primitive : mesh.primitives) {
        if (primitive.attributes.contains("POSITION")) {
            totalVertexCount += gltfModel.accessors.at(primitive.attributes.at("POSITION")).count;
        }

        if (primitive.indices >= 0) {
            totalIndexCount += gltfModel.accessors.at(primitive.indices).count;
        }
    }

    std::vector<Vertex> vertexData;
    vertexData.reserve(totalVertexCount);
    std::vector<uint32_t> indexData;
    indexData.reserve(totalIndexCount);
    for (const tinygltf::Primitive& primitive : mesh.primitives) {
        MeshAssetFormat::SubMeshInfo subMeshInfo;
        if (primitive.attributes.find(kGltfTexCoord0) == primitive.attributes.end()) {
            continue;
        }

        if (primitive.indices < 0) {
            continue;
        }
        const auto posResult = GetAttributePtr<const glm::vec3>(gltfModel, primitive, kGltfPosition);
        const auto texResult =
            GetAttributePtr<const glm::vec2>(gltfModel, primitive, kGltfTexCoord0);
        const auto norResult = GetAttributePtr<const glm::vec3>(gltfModel, primitive, kGltfNormal);
        const auto tanResult = GetAttributePtr<const glm::vec4>(gltfModel, primitive, kGltfTangent);
        if (!posResult.has_value() || !texResult.has_value()) {
            continue;
        }
        const auto posSpan = posResult.value();
        const auto texSpan = texResult.value();
        const auto norSpan = norResult.has_value()
                           ? norResult.value()
                           : core::memory::StridedSpan<const glm::vec3>::MakeZero(posSpan.size());
        auto tanSpan = tanResult.has_value()
                           ? tanResult.value()
                           : core::memory::StridedSpan<const glm::vec4>::MakeZero(posSpan.size());

        subMeshInfo.baseVertexLocation = vertexData.size();
        subMeshInfo.vertexType = VertexType::StandardMesh;

        for (size_t i = 0; i < posSpan.size(); ++i) {
            vertexData.push_back(Vertex{.position = posSpan[i],
                                        .normal = norSpan[i],
                                        .uv = texSpan[i],
                                        .tangent = tanSpan[i]});
        }

        const auto& indexAccessor = gltfModel.accessors[primitive.indices];
        const auto& indexBufferView = gltfModel.bufferViews[indexAccessor.bufferView];
        const auto& indexBuffer = gltfModel.buffers[indexBufferView.buffer];

        const void* indices = reinterpret_cast<const void*>(
            &indexBuffer.data[indexBufferView.byteOffset + indexAccessor.byteOffset]);
        subMeshInfo.indexCount = indexAccessor.count;
        subMeshInfo.baseIndexLocation = indexData.size();
        if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
            const uint32_t* intIndices = reinterpret_cast<const uint32_t*>(indices);
            size_t indexSize = tinygltf::GetComponentSizeInBytes(indexAccessor.componentType) *
                               indexAccessor.count;
            std::span<const uint32_t> indexRnage(intIndices, indexSize);
            indexData.append_range(indexRnage);

        } else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT) {
            const uint16_t* shortIndices = reinterpret_cast<const uint16_t*>(indices);
            for (size_t i = 0; i < indexAccessor.count; ++i) {
                indexData.push_back(static_cast<uint32_t>(shortIndices[i]));
            }
        } else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE) {
            const uint8_t* byteIndices = reinterpret_cast<const uint8_t*>(indices);
            for (size_t i = 0; i < indexAccessor.count; ++i) {
                indexData.push_back(static_cast<uint32_t>(byteIndices[i]));
            }
        } else {
            return std::unexpected(Error{ErrorType::AssetParsingError,
                                         "Unsupported index component type in glTF mesh!"});
        }
        subMeshInfos.push_back(subMeshInfo);
    }

    std::byte* vertexPtr = reinterpret_cast<std::byte*>(vertexData.data());
    return MeshAssetFormat{
        .subMeshes = std::move(subMeshInfos),
        .indexData = std::move(indexData),
        .vertexData = std::vector(vertexPtr, vertexPtr + vertexData.size() * sizeof(Vertex)),
    };
}

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

std::expected<TextureAssetFormat, Error> GLTFImporter::ImportTextureFromTinygltf(
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