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

struct AssetPath {
    std::string value;

    // 편의를 위한 비교 연산자
    bool operator==(const AssetPath& other) const { return value == other.value; }
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

using PropertyId = uint32_t;
constexpr PropertyId ToPropertyID(std::string_view name) {
    // Simple hash function for demonstration purposes
    PropertyId hash = 0;
    for (char c : name) {
        hash = hash * 31 + static_cast<unsigned char>(c);
    }
    return hash;
}

constexpr uint32_t kSetNumberGlobal = 0;
constexpr uint32_t kSetNumberMaterial = 1;
constexpr uint32_t kSetNumberInstance = 2;
}  // namespace core

// std::unordered_map의 Key로 쓰기 위한 해시 특수화
template <>
struct std::hash<core::AssetPath> {
    std::size_t operator()(const core::AssetPath& k) const {
        return std::hash<std::string>{}(k.value);
    }
};