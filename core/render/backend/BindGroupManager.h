#pragma once
#include <vector>

#include "render/resource/ShaderManager.h"
#include "render/resource/MaterialManager.h"

namespace core::render {
class BindGroupManager {
  public:
    BindGroupManager(Device* device,
                     LayoutCache* layoutCache,
                     ShaderManager* shaderManager,
                     MaterialManager* materialManager);

    void UpdateBindGroup(Handle materialHandle);
    wgpu::BindGroup GetBindGroup(Handle materialHandle);

  private:
    Device* m_device;
    LayoutCache* m_layoutCache;
    ShaderManager* m_shaderManager;
    MaterialManager* m_materialManager;
    std::vector<wgpu::BindGroup> m_materialPassBindGroups;
    std::unordered_map<size_t, size_t> m_materialKeyToBindGroupIndex;

};
}