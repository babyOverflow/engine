#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <ranges>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "GLTFImporter.h"

using core::memory::StridedSpan;

namespace core::importer {

const std::string kGltfPosition = "POSITION";
const std::string kGltfTexCoord0 = "TEXCOORD_0";
const std::string kGltfNormal = "NORMAL";
const std::string kGltfTangent = "TANGENT";
const std::string kGltfColor = "COLOR_0";

AssetPath GLTFImporter::ToTextureID(int gltfTextureIndex) {
    return AssetPath{std::format("virtual://tex/{}", gltfTextureIndex)};
}

AssetPath GLTFImporter::ToMaterialID(int gltfMaterialIndex) {
    return AssetPath{std::format("virtual://material/{}", gltfMaterialIndex)};
}

AssetPath GLTFImporter::ToMeshId(int gltfMeshIndex) {
    return AssetPath{std::format("virtual://mesh/{}", gltfMeshIndex)};
}

std::expected<MaterialAssetFormat, Error> GLTFImporter::ImportMaterial(
    const tinygltf::Model& model,
    const tinygltf::Material& gltfMaterial) {
    MaterialAssetFormat assetFormat;

    const auto tryAppendTexture = [&](const auto& textureInfo, const std::string& name) {
        if (textureInfo.index < 0) {
            return;
        }
        assetFormat.SetTexture(name, ToTextureID(textureInfo.index));
    };

    tryAppendTexture(gltfMaterial.pbrMetallicRoughness.baseColorTexture, "baseColorTexture");
    tryAppendTexture(gltfMaterial.pbrMetallicRoughness.metallicRoughnessTexture,
                     "metallicRoughnessTexture");
    tryAppendTexture(gltfMaterial.normalTexture, "normalTexture");
    tryAppendTexture(gltfMaterial.occlusionTexture, "occlusionTexture");
    tryAppendTexture(gltfMaterial.emissiveTexture, "emissiveTexture");

    assetFormat.SetUniform(
        "baseColorFactor",
        glm::vec<4, float>(gltfMaterial.pbrMetallicRoughness.baseColorFactor[0],
                           gltfMaterial.pbrMetallicRoughness.baseColorFactor[1],
                           gltfMaterial.pbrMetallicRoughness.baseColorFactor[2],
                           gltfMaterial.pbrMetallicRoughness.baseColorFactor[3]));
    assetFormat.SetUniform("emissiveFactor", glm::vec<3, float>(gltfMaterial.emissiveFactor[0],
                                                                gltfMaterial.emissiveFactor[1],
                                                                gltfMaterial.emissiveFactor[2]));
    assetFormat.SetUniform("normalScale", static_cast<float>(gltfMaterial.normalTexture.scale));

    assetFormat.SetUniform("metallicFactor",
                           static_cast<float>(gltfMaterial.pbrMetallicRoughness.metallicFactor));
    assetFormat.SetUniform("roughnessFactor",
                           static_cast<float>(gltfMaterial.pbrMetallicRoughness.roughnessFactor));

    return assetFormat;
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

    for (const auto& [idx, material] : gltfModel.materials | std::views::enumerate) {
        result.materials.push_back(
            MaterialResult{ImportMaterial(gltfModel, material).value_or({}), ToMaterialID(idx)});
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
            if (primitive.material >= 0) {
                modelNode.materialIds.push_back(ToMaterialID(primitive.material));
            } else {
                modelNode.materialIds.push_back(AssetPath{"virtual://material/default"});
            }
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

    std::vector<MeshAssetFormat::BufferRange> currentRanges;
    std::vector<MeshAssetFormat::MeshVertexState> vertexStates;
    std::vector<std::byte> vertexData;
    vertexData.reserve(totalVertexCount * (12 + 36));
    std::vector<uint32_t> indexData;
    indexData.reserve(totalIndexCount);
    for (const tinygltf::Primitive& primitive : mesh.primitives) {
        MeshAssetFormat::SubMeshInfo subMeshInfo;
        if (primitive.indices < 0) {
            continue;
        }
        auto posSpanOpt = GetAttributePtr<const glm::vec3>(gltfModel, primitive, kGltfPosition);
        if (!posSpanOpt) {
            continue;
        }

        MeshAssetFormat::MeshVertexState currentState;
        subMeshInfo.bufferRangeStart = currentRanges.size();
        auto posSpan = posSpanOpt.value();
        size_t vertexCount = posSpan.size();

        MeshAssetFormat::MeshBufferSlot posSlot{
            .stepMode = MeshAssetFormat::StepMode::Vertex,
            .stride = sizeof(glm::vec3),
            .attributeCount = 1,
            .attributes = {MeshAssetFormat::MeshAttribute{MeshAssetFormat::VertexFormat::Float32x3,
                                                          Semantic::Position, 0}}};
        currentState.bufferSlots[currentState.slotCount++] = posSlot;

        MeshAssetFormat::BufferRange posRange{
            .offset = static_cast<uint32_t>(vertexData.size()),
            .size = static_cast<uint32_t>(vertexCount * posSlot.stride)};
        vertexData.resize(vertexData.size() + posRange.size);
        std::byte* posDst = vertexData.data() + posRange.offset;

        if (posSpan.stride() == sizeof(glm::vec3)) {
            std::memcpy(posDst, posSpan.GetRawBytePtr(), posRange.size);
        } else {
            for (size_t i = 0; i < vertexCount; ++i) {
                std::memcpy(posDst + (i * posSlot.stride),
                            posSpan.GetRawBytePtr() + (i * posSpan.stride()), sizeof(glm::vec3));
            }
        }
        currentRanges.push_back(posRange);

        auto texSpanOpt = GetAttributePtr<const glm::vec2>(gltfModel, primitive, kGltfTexCoord0);
        if (texSpanOpt) {
            auto texSpan = texSpanOpt.value();
            auto norSpanOpt = GetAttributePtr<const glm::vec3>(gltfModel, primitive, kGltfNormal);
            auto tanSpanOpt = GetAttributePtr<const glm::vec4>(gltfModel, primitive, kGltfTangent);

            // TODO!(#7; sunghyun): Replace MakeZero padding with true dynamic packing. Currently,
            // standard shaders strictly require Normal and Tangent inputs. This bandwidth waste
            // will be resolved once the Shader Permutation system (#7) is implemented.
            auto norSpan = norSpanOpt.has_value()
                               ? norSpanOpt.value()
                               : core::memory::StridedSpan<const glm::vec3>::MakeZero(vertexCount);
            auto tanSpan = tanSpanOpt.has_value()
                               ? tanSpanOpt.value()
                               : core::memory::StridedSpan<const glm::vec4>::MakeZero(vertexCount);

            MeshAssetFormat::MeshBufferSlot surSlot{
                .stepMode = MeshAssetFormat::StepMode::Vertex,
                .stride = 12 + 8 + 16,  // Normal + UV + Tangent
                .attributeCount = 3,
                .attributes = {
                    MeshAssetFormat::MeshAttribute{MeshAssetFormat::VertexFormat::Float32x3,
                                                   Semantic::Normal, 0},
                    MeshAssetFormat::MeshAttribute{MeshAssetFormat::VertexFormat::Float32x2,
                                                   Semantic::TexCoord0, 12},
                    MeshAssetFormat::MeshAttribute{MeshAssetFormat::VertexFormat::Float32x4,
                                                   Semantic::Tangent, 20}}};
            currentState.bufferSlots[currentState.slotCount++] = surSlot;

            MeshAssetFormat::BufferRange surRange{
                .offset = static_cast<uint32_t>(vertexData.size()),
                .size = static_cast<uint32_t>(vertexCount * surSlot.stride)};
            vertexData.resize(vertexData.size() + surRange.size);
            std::byte* surDst = vertexData.data() + surRange.offset;

            struct RawSpan {
                const std::byte* data;
                size_t stride;
            };
            std::array<RawSpan, 3> activeSpans = {
                RawSpan{norSpan.GetRawBytePtr(), norSpan.stride()},
                RawSpan{texSpan.GetRawBytePtr(), texSpan.stride()},
                RawSpan{tanSpan.GetRawBytePtr(), tanSpan.stride()}};

            for (size_t i = 0; i < vertexCount; ++i) {
                std::byte* currentVertDst = surDst + (i * surSlot.stride);
                for (size_t j = 0; j < surSlot.attributeCount; ++j) {
                    size_t payloadSize =
                        MeshAssetFormat::GetVertexFormatSize(surSlot.attributes[j].format);
                    const std::byte* srcPtr = activeSpans[j].data + (i * activeSpans[j].stride);
                    std::memcpy(currentVertDst, srcPtr, payloadSize);
                    currentVertDst += payloadSize;
                }
            }
            currentRanges.push_back(surRange);
        }

        auto colorSpanOpt = GetAttributePtr<const glm::vec4>(gltfModel, primitive, kGltfColor);
        if (colorSpanOpt) {
            auto colorSpan = colorSpanOpt.value();
            MeshAssetFormat::MeshBufferSlot colorSlot{
                .stepMode = MeshAssetFormat::StepMode::Vertex,
                .stride = sizeof(glm::vec4),
                .attributeCount = 1,
                .attributes = {MeshAssetFormat::MeshAttribute{
                    MeshAssetFormat::VertexFormat::Float32x4, Semantic::Color0, 0}},
            };
            currentState.bufferSlots[currentState.slotCount++] = colorSlot;

            MeshAssetFormat::BufferRange colorRange{
                .offset = static_cast<uint32_t>(vertexData.size()),
                .size = static_cast<uint32_t>(vertexCount * colorSlot.stride)};
            vertexData.resize(vertexData.size() + colorRange.size);
            std::byte* colorDst = vertexData.data() + colorRange.offset;

            if (colorSpan.stride() == sizeof(glm::vec4)) {
                std::memcpy(colorDst, colorSpan.GetRawBytePtr(), colorRange.size);
            } else {
                for (size_t i = 0; i < vertexCount; ++i) {
                    std::memcpy(colorDst + (i * colorSlot.stride),
                                colorSpan.GetRawBytePtr() + (i * colorSpan.stride()),
                                sizeof(glm::vec4));
                }
            }
            currentRanges.push_back(colorRange);
        }

        const auto& indexAccessor = gltfModel.accessors[primitive.indices];
        const auto& indexBufferView = gltfModel.bufferViews[indexAccessor.bufferView];
        const auto& indexBuffer = gltfModel.buffers[indexBufferView.buffer];

        const void* indices = reinterpret_cast<const void*>(
            &indexBuffer.data[indexBufferView.byteOffset + indexAccessor.byteOffset]);
        subMeshInfo.indexCount = indexAccessor.count;
        subMeshInfo.indexStart = indexData.size();

        if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT) {
            const uint32_t* intIndices = reinterpret_cast<const uint32_t*>(indices);

            std::span<const uint32_t> indexRange(intIndices, indexAccessor.count);
            indexData.append_range(indexRange);

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

        auto it = std::ranges::find(vertexStates, currentState);
        uint32_t stateIndex = 0;
        if (it != vertexStates.end()) {
            stateIndex = static_cast<uint32_t>(std::distance(vertexStates.begin(), it));
        } else {
            stateIndex = static_cast<uint32_t>(vertexStates.size());
            vertexStates.push_back(currentState);
        }

        subMeshInfo.stateIndex = stateIndex;
        subMeshInfo.bufferRangeCount = currentRanges.size() - subMeshInfo.bufferRangeStart;

        subMeshInfos.push_back(subMeshInfo);
    }

    return MeshAssetFormat{
        .states = std::move(vertexStates),
        .bufferRanges = std::move(currentRanges),
        .subMeshes = std::move(subMeshInfos),
        .indexData = std::move(indexData),
        .vertexData = std::move(vertexData),
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