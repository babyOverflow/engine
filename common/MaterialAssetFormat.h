#pragma once
#include <unordered_map>
#include <vector>
#include "Common.h"

namespace core {
/**
 * @brief 파일(GLTF, JSON 등)과 런타임(Material) 사이를 이어주는 중간 데이터 포맷.
 * GLTF 의존성 없이 순수한 데이터만 담습니다.
 */
struct MaterialAssetFormat {
    // 1. 셰이더 이름 (AssetManager에서 조회하기 위한 Key)
    // 기본값은 PBR로 설정하여 비어있을 경우에 대비
    AssetPath shaderName = {"StandardPBR"};

    // 2. 유니폼 데이터 (Float, Vec3, Vec4, Mat4 등)
    // Key: 셰이더 내의 변수 이름 (예: "u_BaseColorFactor")
    // Value: 직렬화된 바이트 데이터
    struct UniformValue {
        std::vector<uint8_t> data;
    };
    std::unordered_map<std::string, UniformValue> uniforms;

    // 3. 텍스처 바인딩 정보
    // Key: 셰이더 내의 텍스처 슬롯 이름 (예: "u_BaseColorTexture")
    // Value: 텍스처 에셋의 파일 경로 또는 고유 이름 (예: "assets/textures/helmet_base.png")
    std::unordered_map<std::string, AssetPath> textures;

    // --------------------------------------------------------
    // Helper Methods (데이터를 쉽게 넣기 위함)
    // --------------------------------------------------------

    /**
     * @brief 값을 바이트 배열로 변환하여 저장하는 헬퍼 함수
     */
    template <typename T>
    void SetUniform(const std::string& name, const T& value) {
        std::vector<uint8_t> bytes(sizeof(T));
        std::memcpy(bytes.data(), &value, sizeof(T));
        uniforms[name] = {std::move(bytes)};
    }

    /**
     * @brief 텍스처 경로를 설정하는 헬퍼 함수
     */
    void SetTexture(const std::string& slotName, const AssetPath& texturePath) {
        textures[slotName] = texturePath;
    }
};
}  // namespace core