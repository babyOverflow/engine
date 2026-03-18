#pragma once

#include <expected>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

#include <ShaderAssetFormat.h>
#include <slang-com-ptr.h>
#include <slang.h>
#include <span>

namespace slangCompiler {

enum class ErrorType {
    None = 0,
    InitFailed,          // Slang 세션 초기화 실패 (DLL 누락 등)
    IOError,             // 파일 읽기 실패
    CompilationFailed,   // 셰이더 문법 오류, 링크 오류
    EntryPointNotFound,  // "main" 함수를 못 찾음
    ReflectionFailed,    // 리플렉션 데이터 추출 실패 (지원하지 않는 타입 등)
    InternalError,       // 기타 메모리 부족 등
};

struct Error {
    ErrorType type;
    std::string message;
};

struct CompileResult {
    std::vector<core::ShaderAssetFormat::ShaderParameter> parameters;
    std::vector<core::ShaderAssetFormat::Binding> bindings;
    std::vector<core::ShaderAssetFormat::Variable> variables;
    std::vector<core::ShaderAssetFormat::EntryPoint> entryPoints;
    std::vector<uint8_t> sourceBlob;
    std::vector<std::string> nameTable;
    std::vector<uint32_t> indices;
    std::string warning;
};

struct CompilationContext {
    std::string message;
    core::ShaderAssetFormat::ShaderVisibility visibility =
        core::ShaderAssetFormat::ShaderVisibility::None;

    slang::IModule* module;
    std::vector<slang::IEntryPoint*> entryPoints;

    bool Failed(SlangResult result, slang::IBlob* diagnosticBlob);

    void AppendError(slang::IBlob* diagnosticBlob);
};

struct SlangCompilerDesc {
    std::span<std::filesystem::path> paths;
};

class SlangCompiler {
  public:
    static std::expected<SlangCompiler, Error> Create(const SlangCompilerDesc& desc);
    std::expected<CompileResult, Error> CompileFromString(const std::string& slangCode,
                                                          const std::string& entryName);
    std::expected<CompileResult, Error> CompileFromString(const std::string& slangCode);
    std::expected<CompileResult, Error> Compile(const std::string& path,
                                                const std::string& entryName);

  private:
    SlangCompiler(Slang::ComPtr<slang::IGlobalSession> globalSession,
                  std::vector<std::string> paths);
    std::expected<CompileResult, Error> CompileInternal(slang::IComponentType* composedProgram,
                                                        CompilationContext& context);
    std::expected<Slang::ComPtr<slang::ISession>, Error> CreateSession();

    std::vector<std::string> m_paths;
    Slang::ComPtr<slang::IGlobalSession> m_globalSession;
};

}  // namespace slangCompiler