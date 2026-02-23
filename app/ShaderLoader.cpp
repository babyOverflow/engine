#include "ShaderLoader.h"

namespace loader {
std::expected<core::Handle, core::Error> loader::ShaderLoader::LoadShader(const std::string& path) {
    auto resultOrError = core::importer::ShdrImporter::ShdrImport(path);

    if (!resultOrError.has_value()) {
        return std::unexpected(resultOrError.error());
    }

    return m_shaderManager->LoadShader(resultOrError.value());
}
}  // namespace loader
