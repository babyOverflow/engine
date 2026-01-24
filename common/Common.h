#pragma once

#include <string>

namespace core {
enum class ErrorType {
    None = 0,

    IOError,
    OutOfMemory,

    AssetParsingError,
    AssetNotFound,
};

struct Error {
    ErrorType type;
    std::string message;

    static Error IO(std::string message) { return {ErrorType::IOError, message}; }
    static Error Parse(std::string message) { return {ErrorType::AssetParsingError, message}; }
    static Error AssetParsing(std::string message) {
        return {ErrorType::AssetParsingError, message};
    }
};
}  // namespace core