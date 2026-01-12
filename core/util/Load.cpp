#include <format>

#include "Load.h"

namespace core::util {

std::expected<std::string, Error> core::util::ReadFileToString(
    const std::filesystem::path& filepath) {
    std::ifstream fileStream(filepath, std::ios::in | std::ios::binary);
    if (!fileStream) {
        return std::unexpected(Error::IO(std::format("Failed to open: {}.", filepath.string())));
    }
    std::string content;
    fileStream.seekg(0, std::ios::end);
    content.resize(fileStream.tellg());
    fileStream.seekg(0, std::ios::beg);
    fileStream.read(content.data(), content.size());
    fileStream.close();
    return content;
}

std::expected<std::vector<uint8_t>, Error> core::util::ReadFileToByte(
    const std::filesystem::path& filepath) {
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    std::string filename = filepath.string();
    if (!file.is_open()) {
        return std::unexpected(
            Error::IO(std::format("Failed to open: {} does not exist.", filename)));
    }
    std::streamsize fileSize = file.tellg();
    if (fileSize < 0) {
        return std::unexpected(Error::IO(std::format("Failed to read: {}.", filename)));
    }

    size_t count = static_cast<size_t>(fileSize);
    std::vector<uint8_t> buffer(count);
    file.seekg(0, std::ios::beg);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), fileSize)) {
        return std::unexpected(Error::IO(std::format("Failed to read: {}.", filename)));
    }
    return buffer;
}
}  // namespace core::util