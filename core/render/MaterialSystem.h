
#include "Material.h"

namespace core::render {

class MaterialSystem {
  public:
    MaterialSystem() = delete;
    MaterialSystem(Device* device, LayoutCache* layoutCache, PipelineManager* piplineManager);
    ~MaterialSystem() = default;
    MaterialSystem(const MaterialSystem&) = delete;
    MaterialSystem& operator=(const MaterialSystem&) = delete;
    MaterialSystem(MaterialSystem&&) noexcept = default;
    MaterialSystem& operator=(MaterialSystem&&) noexcept = default;

    Material CreateMaterialFromShader(AssetView<ShaderAsset> shaderAsset);

  private:
    Device* m_device;
    LayoutCache* m_layoutCache;
    PipelineManager* m_pipelineManager;
};

}