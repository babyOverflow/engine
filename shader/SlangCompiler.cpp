#include <algorithm>
#include <filesystem>
#include <print>
#include <ranges>

#include "SlangCompiler.h"

using Slang::ComPtr;
using namespace slang;
using sa = core::ShaderAsset;

namespace slangCompiler {

slangCompiler::SlangCompiler::SlangCompiler(ComPtr<IGlobalSession> globalSession,
                                            std::vector<std::string> paths)
    : m_globalSession(std::move(globalSession)), m_paths(paths) {}

std::expected<SlangCompiler, Error> SlangCompiler::Create(const SlangCompilerDesc& desc) {
    ComPtr<IGlobalSession> globalSession;
    {
        SlangGlobalSessionDesc desc{

        };
        SlangResult result = createGlobalSession(&desc, globalSession.writeRef());
        if (SLANG_FAILED(result)) {
            return std::unexpected(Error{ErrorType::InitFailed, "Failed to init Global Session!"});
        }
    }

    std::vector<std::string> paths;

    paths.reserve(desc.paths.size());
    for (const auto& path : desc.paths) {
        paths.emplace_back(path.string());
    }
    return SlangCompiler(std::move(globalSession), paths);
}

std::expected<CompileResult, Error> SlangCompiler::CompileFromString(

    const std::string& slangCode,
    const std::string& entryName) {
    CompilationContext context;
    ComPtr<IBlob> diagnosticBlob;

    ComPtr<ISession> session;
    if (auto result = CreateSession(); result.has_value()) {
        session = result.value();
    } else {
        return std::unexpected(result.error());
    }

    ComPtr<IModule> module;
    {
        IModule* m = session->loadModuleFromSourceString(
            "TempModule", "TempModule.slang", slangCode.data(), diagnosticBlob.writeRef());
        if (m == nullptr) {
            context.AppendError(diagnosticBlob.get());
            return std::unexpected(
                Error{ErrorType::InitFailed, "Failed to load module: " + context.message});
        }
        module = m;
    }

    ComPtr<IEntryPoint> entry;
    if (SLANG_FAILED(module->findEntryPointByName(entryName.data(), entry.writeRef()))) {
        std::string errorMessage = "Error: " + std::string(entryName) + " was not found!\n";
        return std::unexpected(
            Error{ErrorType::EntryPointNotFound, context.message + errorMessage});
    }
    ComPtr<IComponentType> composedProgram;
    {
        IComponentType* componentTypes[] = {module.get(), entry.get()};
        const auto result = session->createCompositeComponentType(
            componentTypes, 2, composedProgram.writeRef(), diagnosticBlob.writeRef());

        if (context.Failed(result, diagnosticBlob.get())) {
            return std::unexpected(Error{ErrorType::CompilationFailed, context.message});
        }
    }

    return CompileInternal(composedProgram.get(), context);
}

std::expected<CompileResult, Error> SlangCompiler::Compile(const std::string& path,
                                                           const std::string& entryName) {
    CompilationContext context;

    ComPtr<ISession> session;
    if (auto result = CreateSession(); result.has_value()) {
        session = result.value();
    } else {
        return std::unexpected(result.error());
    }

    ComPtr<IModule> module;
    ComPtr<IBlob> diagnosticBlob;
    {
        if (!std::filesystem::exists(path)) {
            return std::unexpected(
                Error{ErrorType::IOError, "Failed to find filepath: \"" + path + "\"!\n"});
        }
        IModule* m = session->loadModule(path.data(), diagnosticBlob.writeRef());
        if (m == nullptr) {
            context.AppendError(diagnosticBlob.get());
            return std::unexpected(
                Error{ErrorType::InitFailed, "Failed to load module: " + context.message});
        }
        module = m;
    }

    ComPtr<IEntryPoint> entry;
    if (SLANG_FAILED(module->findEntryPointByName(entryName.data(), entry.writeRef()))) {
        std::string errorMessage = "Error: " + std::string(entryName) + " was not found!\n";
        return std::unexpected(
            Error{ErrorType::EntryPointNotFound, context.message + errorMessage});
    }

    ComPtr<IComponentType> composedProgram;
    {
        IComponentType* componentTypes[] = {module.get(), entry.get()};
        const auto result = session->createCompositeComponentType(
            componentTypes, 2, composedProgram.writeRef(), diagnosticBlob.writeRef());

        if (context.Failed(result, diagnosticBlob.get())) {
            return std::unexpected(Error{ErrorType::CompilationFailed, context.message});
        }
    }

    return CompileInternal(composedProgram.get(), context);
}

static sa::ResourceType MapSlangTypeToResourceType(TypeReflection* type) {
    TypeReflection::Kind kind = type->getKind();

    // 1. Uniform Buffer (ConstantBuffer)
    if (kind == TypeReflection::Kind::ConstantBuffer) {
        return sa::ResourceType::UniformBuffer;
    }

    // 2. Resource (Texture, StorageBuffer 등)
    if (kind == TypeReflection::Kind::Resource) {
        SlangResourceShape shape = type->getResourceShape();

        switch (shape) {
            case SLANG_TEXTURE_2D:
            case SLANG_TEXTURE_CUBE:
            case SLANG_TEXTURE_2D_ARRAY:
                return sa::ResourceType::Texture;

            case SLANG_STRUCTURED_BUFFER:
            case SLANG_BYTE_ADDRESS_BUFFER:
                // Slang은 RWStructuredBuffer와 StructuredBuffer를 접근 권한으로 구분함
                // 여기서는 간단히 Storage로 매핑하되, 정교한 구분은 AccessProps 체크 필요
                return sa::ResourceType::StorageBuffer;

            default:
                return sa::ResourceType::Unknown;
        }
    }

    // 3. Sampler
    if (kind == TypeReflection::Kind::SamplerState) {
        return sa::ResourceType::Sampler;
    }

    return sa::ResourceType::Unknown;
}

constexpr uint32_t kInvalidSetNumber = -1;
inline bool IsValidSet(uint32_t setNumber) {
    return setNumber != kInvalidSetNumber;
}

static void ReflectParameter(VariableLayoutReflection* varLayout,
                             uint32_t currentSet,
                             uint32_t bindingOffset,
                             std::string& prefix,
                             std::vector<sa::Binding>& outBindings) {
    TypeLayoutReflection* typeLayout = varLayout->getTypeLayout();
    TypeReflection::Kind kind = typeLayout->getKind();

    const char* rawName = varLayout->getName();
    std::string varName = rawName == nullptr ? "" : std::string(rawName);

    std::string fullName;
    if (prefix.empty()) {
        fullName = varName;
    } else if (!varName.empty()) {
        fullName = prefix + "." + varName;
    } else {
        fullName = prefix;
    }

    if (kind == TypeReflection::Kind::ParameterBlock) {
        TypeLayoutReflection* innerType = typeLayout->getElementTypeLayout();
        size_t spaceOffset =
            varLayout->getOffset(slang::ParameterCategory::SubElementRegisterSpace);

        uint32_t newSet = varLayout->getBindingSpace();
        if (spaceOffset != SLANG_UNBOUNDED_SIZE) {
            newSet = static_cast<uint32_t>(spaceOffset);
        } else {
            newSet = static_cast<uint32_t>(varLayout->getBindingSpace());
        }

        uint32_t fieldCount = innerType->getFieldCount();
        for (uint32_t i = 0; i < fieldCount; ++i) {
            VariableLayoutReflection* fieldVar = innerType->getFieldByIndex(i);
            ReflectParameter(fieldVar, newSet, 0, fullName, outBindings);
        }

        uint32_t implicitSize = innerType->getSize();
        if (implicitSize > 0) {
            size_t cbufferIndex =
                varLayout->getOffset(slang::ParameterCategory::DescriptorTableSlot);
            if (cbufferIndex == SLANG_UNBOUNDED_SIZE) {
                cbufferIndex = 0;
            }

            sa::Binding cbufferBinding{
                .set = newSet,
                .binding = static_cast<uint32_t>(cbufferIndex),
                .bufferSize = implicitSize,
                .resourceType = sa::ResourceType::UniformBuffer,
                .visibility = sa::ShaderVisibility::All,
            };

            auto name = fullName + "_CBuffer";
            std::strncpy(cbufferBinding.name, name.c_str(), sizeof(cbufferBinding.name));

            outBindings.push_back(cbufferBinding);
        }
        return;
    } else if (kind == TypeReflection::Kind::Struct) {
        uint32_t baseIndex = 0;
        if (varLayout->getBindingIndex() != -1) {
            baseIndex += varLayout->getBindingIndex();
        }

        uint32_t fieldCount = typeLayout->getFieldCount();
        for (uint32_t i = 0; i < fieldCount; ++i) {
            VariableLayoutReflection* fieldRef = typeLayout->getFieldByIndex(i);
            ReflectParameter(fieldRef, currentSet, baseIndex, fullName, outBindings);
        }
        return;
    }

    if (varLayout->getBindingIndex() != -1) {
        size_t bufferSize = typeLayout->getSize();
        if (kind == TypeReflection::Kind::ConstantBuffer) {
            TypeLayoutReflection* elementLayout = typeLayout->getElementTypeLayout();
            bufferSize = elementLayout->getSize();
        }
        uint32_t set = currentSet;
        if (set == -1) {
            size_t spaceOffset = varLayout->getOffset(slang::ParameterCategory::RegisterSpace);
            set = (spaceOffset != -SLANG_UNBOUNDED_SIZE) ? (uint32_t)spaceOffset : 0;
        }
        sa::Binding binding{
            .set = set,
            .binding = bindingOffset + varLayout->getBindingIndex(),
            .bufferSize = static_cast<uint32_t>(bufferSize),
            .resourceType = MapSlangTypeToResourceType(typeLayout->getType()),
            .visibility = sa::ShaderVisibility::All,
        };

        std::strncpy(binding.name, fullName.c_str(), sizeof(binding.name) - 1);

        if (binding.resourceType != sa::ResourceType::Unknown) {
            outBindings.push_back(binding);
        }
    }
}

std::vector<sa::Binding> GenerateBindingsFromLayout(slang::ProgramLayout* layout) {
    std::vector<sa::Binding> bindings;
    uint32_t paramCount = layout->getParameterCount();
    for (uint32_t i = 0; i < paramCount; ++i) {
        VariableLayoutReflection* varRefl = layout->getParameterByIndex(i);
        std::string empty("");
        // Search recursively binding info from slang reflection tree and store in the binsings.
        ReflectParameter(varRefl, kInvalidSetNumber, 0, empty, bindings);
    }

    std::ranges::sort(bindings, [](sa::Binding& a, sa::Binding& b) -> bool {
        if (a.set != b.set) {
            return a.set < b.set;
        }
        return a.binding < b.binding;
    });

    return bindings;
}

std::expected<CompileResult, Error> slangCompiler::SlangCompiler::CompileInternal(
    slang::IComponentType* composedProgram,
    CompilationContext& context) {
    ComPtr<IBlob> diagnosticBlob;
    ComPtr<IComponentType> linkedProgram;
    {
        const auto result =
            composedProgram->link(linkedProgram.writeRef(), diagnosticBlob.writeRef());

        if (context.Failed(result, diagnosticBlob.get())) {
            return std::unexpected(Error{ErrorType::CompilationFailed, context.message});
        }
    }

    ComPtr<IBlob> codeBlob;
    {
        const auto result =
            linkedProgram->getEntryPointCode(0, 0, codeBlob.writeRef(), diagnosticBlob.writeRef());
        if (context.Failed(result, diagnosticBlob.get())) {
            return std::unexpected(Error{ErrorType::CompilationFailed, context.message});
        }
    }

    ProgramLayout* layout = linkedProgram->getLayout();

    std::vector<sa::Binding> bindings = GenerateBindingsFromLayout(layout);

    std::vector<uint8_t> code(codeBlob->getBufferSize());
    memcpy(code.data(), codeBlob->getBufferPointer(), codeBlob->getBufferSize());
    sa::Header header = {
        .bindingCount = static_cast<uint16_t>(bindings.size()),
        .shaderSize = static_cast<uint32_t>(code.size()),
    };
    // temp return
    return std::expected<CompileResult, Error>(CompileResult{
        .header = header, .bindings = bindings, .sourceBlob = code, .warning = context.message});
}

std::expected<Slang::ComPtr<slang::ISession>, Error> SlangCompiler::CreateSession() {
    ComPtr<ISession> session;
    {
        const TargetDesc targetDesc{
            .format = SLANG_WGSL,
            .profile = m_globalSession->findProfile("sm_6_0"),
            .flags = SLANG_TARGET_FLAG_GENERATE_WHOLE_PROGRAM,
        };
        std::vector<const char*> searchPaths;
        for (const auto& path : m_paths) {
            searchPaths.push_back(path.c_str());
            std::println(" - \"{}\"", path);
        }
        std::vector<CompilerOptionEntry> options;
        options.emplace_back(slang::CompilerOptionEntry{
            .name = slang::CompilerOptionName::MatrixLayoutColumn,
            .value = {
                .kind = slang::CompilerOptionValueKind::Int,
                .intValue0 = 1  // 여기서 Column-Major 설정
            }});
        const SessionDesc sessionDesc{
            .targets = &targetDesc,
            .targetCount = 1,
            .searchPaths = searchPaths.data(),
            .searchPathCount = static_cast<SlangInt>(searchPaths.size()),
            .compilerOptionEntries = options.data(),
            .compilerOptionEntryCount = static_cast<uint32_t>(options.size()),
        };

        const auto result = m_globalSession->createSession(sessionDesc, session.writeRef());
        if (SLANG_FAILED(result)) {
            return std::unexpected(Error{ErrorType::InitFailed, "Failed to init Session!"});
        }
    }
    return session;
}

bool slangCompiler::CompilationContext::Failed(SlangResult result, slang::IBlob* diagnosticBlob) {
    if (diagnosticBlob != nullptr) {
        message += std::string(static_cast<const char*>(diagnosticBlob->getBufferPointer()),
                               diagnosticBlob->getBufferSize());
        if (!message.empty() && message.back() != '\n') {
            message += "\n";
        }
    }
    return SLANG_FAILED(result);
}

void CompilationContext::AppendError(slang::IBlob* diagnosticBlob) {
    Failed(SLANG_FAIL, diagnosticBlob);
}

}  // namespace slangCompiler