#pragma once
#include <expected>

#include <Common.h>
#include <render/ShaderManager.h>

namespace loader {
class ShaderLoader {
  public:
    ShaderLoader(core::render::ShaderManager* shaderManager) : m_shaderManager(shaderManager) {}
    std::expected<core::Handle, core::Error> LoadShader(const std::string& path);

  private:
    core::render::ShaderManager* m_shaderManager = nullptr;
};
}  // namespace loader