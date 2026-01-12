#pragma once
#include <expected>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "Common.h"

namespace core::util {
std::expected<std::string, Error> ReadFileToString(const std::filesystem::path& filepath);


std::expected<std::vector<uint8_t>, Error> ReadFileToByte(const std::filesystem::path& filepath);


}  // namespace myapp::util