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

struct Handle {
    static constexpr uint32_t kInvalidGen = -1;
    uint32_t index = 0;
    uint32_t generation = kInvalidGen;
    bool operator==(const Handle& other) const {
        return index == other.index && generation == other.generation;
    }
    bool IsValid() const { return generation != kInvalidGen; }
};

/**
 * @brief AssetView Smart pointer like structure for assets
 * @tparam T
 */
template <typename T>
struct AssetView {
    T* data = nullptr;
    Handle handle;

    T* operator->() const { return data; }
    T& operator*() const { return *data; }

    bool IsValid() const { return data != nullptr && handle.IsValid(); }

    operator bool() const { return IsValid(); }

    bool operator==(const AssetView<T>& other) const {
        return handle == other.handle && data == other.data;
    }

    T* Get() const { return data; }
};
}  // namespace core