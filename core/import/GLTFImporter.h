#pragma once
#include <glm/glm.hpp>
#include <tiny_gltf.h>


#include "Importer.h"

#include "ResourcePool.h"

#include "render/Mesh.h"

namespace core::importer {



struct GLTFImportResult {
    std::vector<TextureResult> textures;
    std::vector<MaterialResult> materials;
    std::vector<MeshResult> meshes;
    ModelAssetFormat modelAsset;
};

glm::mat4 GetNodeMatrix(const tinygltf::Node& node);

class GLTFImporter {
  public:
    // static std::expected<core::render::Model, Error> ImportFromFile(AssetManager* assetManager,
    //                                                                 const render::Device* device,
    //                                                                 const std::string& filePath);

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

    static std::expected<GLTFImportResult, Error> ImportFromFile(const std::string& filePath);
    static std::expected<TextureAssetFormat, Error> ImportTextureFromTinygltf(
        const tinygltf::Model& model,
        const tinygltf::Image& image);
    static std::expected<MeshAssetFormat, Error> ImportMesh(const tinygltf::Model& model,
                                                            const tinygltf::Mesh& mesh);
    static std::expected<MaterialAssetFormat, Error> ImportMaterial(
        const tinygltf::Model& model,
        const tinygltf::Material& gltfMaterial);

    static AssetPath ToTextureID(int gltfTextureIndex);
  private:
    static AssetPath ToMaterialID(int gltfMaterialIndex);
    static AssetPath ToMeshId(int gltfMeshIndex);
};
}  // namespace core::importer