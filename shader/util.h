#pragma once
#include <filesystem>
#include "SlangCompiler.h"

bool WriteAssetToFile(const std::filesystem::path& outputPath,
                      const slangCompiler::CompileResult& result);