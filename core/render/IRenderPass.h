#pragma once
#include <string>
#include <string_view>
#include <unordered_map>
#include "PipelineManager.h"
#include "AssetManager.h"

inline bool operator==(const wgpu::Extent3D& lhs, const wgpu::Extent3D& rhs) {
    return lhs.width == rhs.width && lhs.height == rhs.height &&
           lhs.depthOrArrayLayers == rhs.depthOrArrayLayers;
}

namespace core::render {

struct RenderIntent {
    Handle meshHandle;
    uint32_t subMeshIndex;
    Handle materialHandle;
    uint32_t transformIndex;
    PipelineKey pipelineKey;
    // TODO(#10): Populate 64-bit sort key for Radix Sorting
    wgpu::BindGroup bindGroup;
    union SortKey {
        uint64_t hash;
        struct {
            uint64_t passId : 8;
            uint64_t pipelineId : 16;
            uint64_t mateiralId : 16;
            uint64_t depth : 24;  // For depth sorting (e.g., transparent objects)
        } bits;
    } sortKey;
};

static_assert(sizeof(RenderIntent::SortKey) == sizeof(uint64_t), "SortKey must be exactly 64 bits");

class BlackBoard {
  public:
    void Set(const std::string& key, Handle value);
    Handle Get(const std::string& key) const;

  private:
    std::unordered_map<std::string, Handle> m_data;
};

class IRenderPass;
class PassSetupContext {
    friend class RenderGraph;

  public:
    constexpr static Handle kSceneColorHandle{.index = 0, .generation = 0};
    constexpr static char const* kSceneColorName = "SceneColor";

    struct RelativeSize {
        float widthRatio = 0.0f;
        float heightRatio = 0.0f;
        friend bool operator==(const RelativeSize& lhs, const RelativeSize& rhs) {
            return lhs.widthRatio == rhs.widthRatio && lhs.heightRatio == rhs.heightRatio;
        }
    };

    using TargetSize = std::variant<wgpu::Extent3D, RelativeSize>;

    struct TextureDescriptor {
        std::string name;
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

    struct ColorAttachment {
        wgpu::LoadOp loadOp = wgpu::LoadOp::Clear;
        wgpu::StoreOp storeOp = wgpu::StoreOp::Store;
        wgpu::Color clearValue = {0.0f, 0.0f, 0.0f, 1.0f};
    };

    struct DepthStencilAttachment {
        wgpu::LoadOp depthLoadOp = wgpu::LoadOp::Clear;
        wgpu::StoreOp depthStoreOp = wgpu::StoreOp::Store;
        float depthClearValue = 1.0f;
        wgpu::LoadOp stencilLoadOp = wgpu::LoadOp::Clear;
        wgpu::StoreOp stencilStoreOp = wgpu::StoreOp::Store;
        uint32_t stencilClearValue = 0;
    };

    struct SubResource {
        PassSetupContext::TextureDescriptor textureDesc;
        std::vector<uint32_t> readPassIds;
        struct WriteInfo {
            uint32_t passId;
            std::variant<ColorAttachment, DepthStencilAttachment> attach;
        };
        std::vector<WriteInfo> writePassInfos;
        uint32_t actualResource;
    };

    PassSetupContext(Device* device, const wgpu::SurfaceConfiguration& surfaceConfig);

    Handle DeclareTexture(const TextureDescriptor& desc);

    void RegisterColorOutput(Handle texture, const ColorAttachment& attach);
    void RegisterDepthStencil(Handle texture, const DepthStencilAttachment& attach);
    void RegisterTextureRead(Handle texture);

    BlackBoard& GetBlackBoard() { return m_blackBoard; }

    void SetPassID(uint32_t id) { m_currentPassId = id; }

  private:
    SubResource& GetSceneColorSubResource() { return m_subResources[kSceneColorHandle.index]; }

    Device* m_device;
    uint32_t m_currentPassId;
    wgpu::SurfaceConfiguration m_surfaceConfig;
    std::vector<SubResource> m_subResources;
    BlackBoard m_blackBoard;
};

class IRenderPass {
  public:
    IRenderPass() = default;
    virtual ~IRenderPass() = default;
    virtual void Execute(wgpu::RenderPassEncoder encoder,
                         std::span<RenderIntent> context,
                         const AssetRegistry& assetRegistry,
                         std::span<const wgpu::RenderPipeline> pipelines) = 0;
    virtual void Setup(PassSetupContext& context) = 0;

    virtual std::string GetPassName() = 0;
};

struct transparent_string_hash {
    using is_transparent = void;

    size_t operator()(std::string_view sv) const noexcept {
        return std::hash<std::string_view>{}(sv);
    }
    size_t operator()(const std::string& str) const noexcept {
        return std::hash<std::string>{}(str);
    }
    size_t operator()(const char* ptr) const noexcept { return std::hash<std::string_view>{}(ptr); }
};

class PassManager {
  public:
    template <typename T>
    uint8_t RegisterPass(const std::string& passName) {
        assert(m_nameToId.find(passName) == m_nameToId.end() && "Pass name collision!");
        assert(m_nextId < 255 && "Pass ID limit exceeded!");
        m_nameToId[passName] = m_nextId;
        m_IdToName[m_nextId] = passName;
        m_creators[m_nextId] = []() { return std::make_unique<T>(); };
        return m_nextId++;
    }

    uint8_t GetPassID(const std::string_view passName) const;
    std::unique_ptr<IRenderPass> CreatePass(uint8_t id) const;

  private:
    uint8_t m_nextId = 0;
    std::unordered_map<uint8_t, std::string> m_IdToName;
    std::unordered_map<std::string, uint8_t, transparent_string_hash, std::equal_to<>> m_nameToId;
    std::array<std::function<std::unique_ptr<IRenderPass>()>, 255> m_creators;
};
}  // namespace core::render

namespace std {

template <>
struct hash<core::render::PassSetupContext::RelativeSize> {
    using RelativeSize = core::render::PassSetupContext::RelativeSize;
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
struct hash<core::render::PassSetupContext::TextureDescriptor> {
    using TextureDescriptor = core::render::PassSetupContext::TextureDescriptor;
    size_t operator()(const TextureDescriptor& desc) const noexcept {
        size_t seed = 0;

        // operator== 에 사용된 멤버들만 추출하여 결합
        wgx::hash_combine(seed, std::hash<int>{}(static_cast<int>(desc.usage)));
        wgx::hash_combine(seed, std::hash<int>{}(static_cast<int>(desc.dimension)));

        // TargetSize의 해시 구현이 필요합니다. (예시)
        wgx::hash_combine(seed, std::hash<core::render::PassSetupContext::TargetSize>{}(desc.size));

        wgx::hash_combine(seed, std::hash<int>{}(static_cast<int>(desc.format)));
        wgx::hash_combine(seed, std::hash<uint32_t>{}(desc.mipLevelCount));
        wgx::hash_combine(seed, std::hash<uint32_t>{}(desc.sampleCount));

        return seed;
    }
};
}  // namespace std