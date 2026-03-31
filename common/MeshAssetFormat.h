#pragma once

#include <glm/glm.hpp>
#include <array>

#include "Common.h"

namespace core {

struct MeshAssetFormat {
    inline constexpr static uint32_t kInvalidIndex = -1;

    enum class VertexFormat : uint8_t { 
        Float32,
        Float32x2,
        Float32x3,
        Float32x4,
        Undefined = 255 };

    enum class StepMode : uint8_t { Vertex, Instance, Undefined = 255 };

    struct MeshAttribute {
        VertexFormat format;
        Semantic semantic;
        uint64_t offset;

        bool operator==(const MeshAttribute&) const = default;
    };

    struct MeshBufferSlot {
        StepMode stepMode;
        uint32_t stride;
        uint8_t attributeCount = 0;
        std::array<MeshAttribute, 8> attributes = {};

        bool operator==(const MeshBufferSlot&) const = default;
    };

    struct MeshVertexState {
        uint8_t slotCount = 0;
        std::array<MeshBufferSlot, 4> bufferSlots = {};

        bool operator==(const MeshVertexState&) const = default;
    };

    struct BufferRange {
        uint32_t offset;
        uint32_t size;
    };

    struct SubMeshInfo {
        uint32_t indexCount;
        uint32_t indexStart;
        uint32_t stateIndex;
        uint32_t bufferRangeStart;  // span 대신 시작 인덱스
        uint32_t bufferRangeCount;
    };

    static constexpr size_t GetVertexFormatSize(VertexFormat format) {
        switch (format) {
            case VertexFormat::Float32x2:
                return 8;
            case VertexFormat::Float32x3:
                return 12;
            case VertexFormat::Float32x4:
                return 16;
            default:
                assert(false && "Unknown vertex format size");
                return 0;
        }
    }

    std::vector<MeshVertexState> states;
    std::vector<BufferRange> bufferRanges;
    std::vector<SubMeshInfo> subMeshes;
    std::vector<uint32_t> indexData;
    std::vector<std::byte> vertexData;
};
}  // namespace core