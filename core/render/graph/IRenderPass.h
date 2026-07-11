#pragma once
#include <string>
#include <string_view>
#include <unordered_map>
#include <ShaderInterop.h>

#include "wgx.h"
#include "AssetManager.h"

inline bool operator==(const wgpu::Extent3D& lhs, const wgpu::Extent3D& rhs) {
    return lhs.width == rhs.width && lhs.height == rhs.height &&
           lhs.depthOrArrayLayers == rhs.depthOrArrayLayers;
}

namespace core::render {

struct RenderIntent {
    uint32_t transformIndex;
    wgpu::RenderPipeline pipeline;
    wgpu::Buffer vertexBuffer;
    wgpu::Buffer indexBuffer;
    MeshAssetFormat::SubMeshInfo subMeshInfo;
    std::span<const MeshAssetFormat::BufferRange> bufferRange;
    // TODO(#10): Populate 64-bit sort key for Radix Sorting
    wgpu::BindGroup bindGroup;
    uint64_t sortKey;

    // Helper to generate a deterministic, endian-independent key
    static constexpr uint64_t CreateOpaqueKey(uint64_t pipeline,
                                              uint64_t material,
                                              uint64_t mesh,
                                              uint64_t depth) {
        // Bit masks to prevent accidental overflow into adjacent fields
        constexpr uint64_t PIPELINE_MASK = 0xFFF;   // 12 bits
        constexpr uint64_t MATERIAL_MASK = 0x3FFF;  // 14 bits
        constexpr uint64_t MESH_MASK = 0x3FFF;      // 14 bits
        constexpr uint64_t DEPTH_MASK = 0xFFFF;     // 16 bits

        // Shift values to their designated logical MSB->LSB positions
        return ((pipeline & PIPELINE_MASK) << 44) | ((material & MATERIAL_MASK) << 30) |
               ((mesh & MESH_MASK) << 16) | (depth & DEPTH_MASK);
    }
};

class BlackBoard {
  public:
    void Set(const std::string& key, Handle value);
    void Set(PropertyId key, Handle value);
    Handle Get(const std::string& key) const;
    Handle Get(PropertyId key) const;

  private:
    std::unordered_map<uint64_t, Handle> m_data;
};

enum class PassFlags : uint32_t {
    None = 0,
    DisableBlend = 1 << 0,  // 이 패스에서는 머티리얼의 블렌드 설정을 무시하고 강제로 끔
    ClearTargets = 1 << 1,
};

struct PassTargetState {
    PassFlags flags = PassFlags::None;
    std::vector<wgpu::TextureFormat> colorTargetFormats;
    wgpu::TextureFormat depthStencilFormat = wgpu::TextureFormat::Undefined;
};

struct RelativeSize {
    float widthRatio = 0.0f;
    float heightRatio = 0.0f;
    friend bool operator==(const RelativeSize& lhs, const RelativeSize& rhs) {
        return lhs.widthRatio == rhs.widthRatio && lhs.heightRatio == rhs.heightRatio;
    }
};

using TargetSize = std::variant<wgpu::Extent3D, RelativeSize>;

struct TextureDescriptor {
    std::string label;
    wgpu::TextureUsage usage = wgpu::TextureUsage::None;
    wgpu::TextureDimension dimension = wgpu::TextureDimension::Undefined;
    TargetSize size = RelativeSize{1.0, 1.0};
    wgpu::TextureFormat format = wgpu::TextureFormat::Undefined;
    uint32_t mipLevelCount = 1;
    uint32_t sampleCount = 1;
    size_t viewFormatCount = 0;
    wgpu::TextureFormat const* viewFormats = nullptr;

    friend bool operator==(const TextureDescriptor& lhs, const TextureDescriptor& rhs) {
        // 1. 프리미티브 필드들 비교
        bool baseEquals = lhs.usage == rhs.usage && lhs.dimension == rhs.dimension &&
                          lhs.format == rhs.format && lhs.mipLevelCount == rhs.mipLevelCount &&
                          lhs.sampleCount == rhs.sampleCount;

        if (!baseEquals) {
            return false;
        }

        // 2. size (std::variant) 수동 비교
        if (lhs.size.index() != rhs.size.index()) {
            return false;
        }

        if (std::holds_alternative<RelativeSize>(lhs.size)) {
            return std::get<RelativeSize>(lhs.size) == std::get<RelativeSize>(rhs.size);
        }

        const auto& extA = std::get<wgpu::Extent3D>(lhs.size);
        const auto& extB = std::get<wgpu::Extent3D>(rhs.size);
        return extA.width == extB.width && extA.height == extB.height &&
               extA.depthOrArrayLayers == extB.depthOrArrayLayers;
    }
    friend bool operator<(const TextureDescriptor& lhs, const TextureDescriptor& rhs) {
        if (lhs.usage != rhs.usage) {
            return lhs.usage < rhs.usage;
        }
        if (lhs.dimension != rhs.dimension) {
            return lhs.dimension < rhs.dimension;
        }
        if (lhs.format != rhs.format) {
            return lhs.format < rhs.format;
        }
        if (lhs.mipLevelCount != rhs.mipLevelCount) {
            return lhs.mipLevelCount < rhs.mipLevelCount;
        }
        if (lhs.sampleCount != rhs.sampleCount) {
            return lhs.sampleCount < rhs.sampleCount;
        }

        // Variant (size) 비교
        if (lhs.size.index() != rhs.size.index()) {
            return lhs.size.index() < rhs.size.index();
        }

        if (std::holds_alternative<RelativeSize>(lhs.size)) {
            const auto& a = std::get<RelativeSize>(lhs.size);
            const auto& b = std::get<RelativeSize>(rhs.size);
            if (a.widthRatio != b.widthRatio) {
                return a.widthRatio < b.widthRatio;
            }
            return a.heightRatio < b.heightRatio;
        } else {
            const auto& a = std::get<wgpu::Extent3D>(lhs.size);
            const auto& b = std::get<wgpu::Extent3D>(rhs.size);
            if (a.width != b.width) {
                return a.width < b.width;
            }
            if (a.height != b.height) {
                return a.height < b.height;
            }
            return a.depthOrArrayLayers < b.depthOrArrayLayers;
        }
    }
};

struct ColorAttachmentInfo {
    wgpu::LoadOp loadOp = wgpu::LoadOp::Clear;
    wgpu::StoreOp storeOp = wgpu::StoreOp::Store;
    wgpu::Color clearValue = {0.0f, 0.0f, 0.0f, 1.0f};
};

struct DepthStencilAttachmentInfo {
    wgpu::LoadOp depthLoadOp = wgpu::LoadOp::Clear;
    wgpu::StoreOp depthStoreOp = wgpu::StoreOp::Store;
    float depthClearValue = 1.0f;
    wgpu::LoadOp stencilLoadOp = wgpu::LoadOp::Clear;
    wgpu::StoreOp stencilStoreOp = wgpu::StoreOp::Store;
    uint32_t stencilClearValue = 0;
};

struct ColorAttachment {
    Handle virTextureHandle;
    ColorAttachmentInfo attachemntInfo;
};

struct DepthStencilAttachment {
    Handle virTextureHandle;
    DepthStencilAttachmentInfo attachmentInfo;
};

struct SubResource {
    TextureDescriptor textureDesc;
    struct ReadInfo {
        uint32_t passId;
    };
    std::vector<ReadInfo> readPassInfos;
    struct WriteInfo {
        uint32_t passId;
    };
    std::vector<WriteInfo> writePassInfos;
    uint32_t actualResource;
};

struct LocalTexture {
    std::string name;
    TextureDescriptor textureDesc;
};

struct PassReadInfo {
    Handle virTexture;
    std::optional<wgpu::TextureViewDescriptor> viewDesc;
};

struct PassSetupContext {
    constexpr static Handle kSceneColorHandle{.index = 0, .generation = 0};
    constexpr static char const* kSceneColorName = "SceneColor";

    Handle DeclareTexture(const std::string& bindingName, const TextureDescriptor& desc);

    void RegisterPassOutputs(
        std::vector<ColorAttachment> colorAttachments,
        std::optional<DepthStencilAttachment> depthStencilAttachment = std::nullopt);

    void RegisterTextureRead(Handle texture);
    void RegisterTextureRead(Handle texture, wgpu::TextureViewDescriptor desc);

    Handle GetResourceHandle(const std::string& name);

    std::vector<LocalTexture> m_declaredTextures = {{"SceneColor"}};
    std::vector<ColorAttachment> m_colorAttachments;
    std::optional<DepthStencilAttachment> m_depthStencilAttachment;

    std::vector<LocalTexture> m_requiredTextures = {{"SceneColor"}};
    std::vector<PassReadInfo> m_readTextures;
};

struct PassExecuteContext {
    std::span<RenderIntent> intents;
    const AssetRegistry& assetRegistry;
    wgpu::RenderPipeline proceduralPipeline;
};

class IRenderPass {
  public:
    IRenderPass() = default;
    virtual ~IRenderPass() = default;
    virtual void Execute(wgpu::RenderPassEncoder encoder,
                         const PassExecuteContext& executeContext) = 0;

    virtual void Setup(PassSetupContext& context) = 0;
};

struct transparent_string_hash {
    using is_transparent = void;

    size_t operator()(const std::string_view sv) const noexcept {
        return std::hash<std::string_view>{}(sv);
    }
    size_t operator()(const std::string& str) const noexcept {
        return std::hash<std::string>{}(str);
    }
    size_t operator()(const char* ptr) const noexcept { return std::hash<std::string_view>{}(ptr); }
};

class PassManager {
  public:
    static constexpr uint32_t kMaxPasses = 255;

    template <typename T>
    uint8_t RegisterPass(const std::string& passName) {
        assert(m_nameToId.find(passName) == m_nameToId.end() && "Pass name collision!");
        assert(m_nextId < kMaxPasses && "Pass ID limit exceeded!");
        m_nameToId[passName] = m_nextId;
        m_IdToName[m_nextId] = passName;
        m_passes[m_nextId] = std::make_unique<T>();
        return m_nextId++;
    }

    uint8_t GetPassID(const std::string_view passName) const;
    IRenderPass* GetPass(uint8_t id) const;
    const std::string& GetPassName(uint8_t id) const;

  private:
    uint8_t m_nextId = 0;
    std::unordered_map<uint8_t, std::string> m_IdToName;
    std::unordered_map<std::string, uint8_t, transparent_string_hash, std::equal_to<>> m_nameToId;
    std::array<std::unique_ptr<IRenderPass>, kMaxPasses> m_passes;
};
}  // namespace core::render

namespace std {

template <>
struct hash<core::render::RelativeSize> {
    using RelativeSize = core::render::RelativeSize;
    size_t operator()(const RelativeSize& size) const noexcept {
        size_t seed = 0;
        wgx::hash_combine(seed, std::hash<float>{}(size.widthRatio));
        wgx::hash_combine(seed, std::hash<float>{}(size.heightRatio));
        return seed;
    }
};

// 2. wgpu::Extent3D 해시 특수화 (WebGPU 구조체)
template <>
struct hash<wgpu::Extent3D> {
    size_t operator()(const wgpu::Extent3D& extent) const noexcept {
        size_t seed = 0;
        wgx::hash_combine(seed, std::hash<uint32_t>{}(extent.width));
        wgx::hash_combine(seed, std::hash<uint32_t>{}(extent.height));
        wgx::hash_combine(seed, std::hash<uint32_t>{}(extent.depthOrArrayLayers));
        return seed;
    }
};

template <>
struct hash<core::render::TextureDescriptor> {
    using TextureDescriptor = core::render::TextureDescriptor;
    size_t operator()(const TextureDescriptor& desc) const noexcept {
        size_t seed = 0;

        // operator== 에 사용된 멤버들만 추출하여 결합
        wgx::hash_combine(seed, std::hash<int>{}(static_cast<int>(desc.usage)));
        wgx::hash_combine(seed, std::hash<int>{}(static_cast<int>(desc.dimension)));

        // TargetSize의 해시 구현이 필요합니다. (예시)
        wgx::hash_combine(seed, std::hash<core::render::TargetSize>{}(desc.size));

        wgx::hash_combine(seed, std::hash<int>{}(static_cast<int>(desc.format)));
        wgx::hash_combine(seed, std::hash<uint32_t>{}(desc.mipLevelCount));
        wgx::hash_combine(seed, std::hash<uint32_t>{}(desc.sampleCount));

        return seed;
    }
};
}  // namespace std
