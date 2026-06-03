#pragma once
#include <unordered_map>
#include <vector>
#include "Common.h"

namespace core {
struct MaterialAssetFormat {
    AssetPath shaderName = {"StandardPBR"};

    struct UniformValue {
        std::vector<uint8_t> data;
    };
    std::unordered_map<std::string, UniformValue> uniforms;

    std::unordered_map<std::string, AssetPath> textures;

    std::vector<std::string> passNames;


    template <typename T>
    void SetUniform(const std::string& name, const T& value) {
        std::vector<uint8_t> bytes(sizeof(T));
        std::memcpy(bytes.data(), &value, sizeof(T));
        uniforms[name] = {std::move(bytes)};
    }

    void SetTexture(const std::string& slotName, const AssetPath& texturePath) {
        textures[slotName] = texturePath;
    }
};
}  // namespace core