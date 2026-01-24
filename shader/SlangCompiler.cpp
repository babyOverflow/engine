#include <algorithm>
#include <filesystem>
#include <print>
#include <ranges>

#include "SlangCompiler.h"

using Slang::ComPtr;
using namespace slang;
using sa = core::ShaderAssetFormat;

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

constexpr uint32_t kInvalidSetNumber = -1;
inline bool IsValidSet(uint32_t setNumber) {
    return setNumber != kInvalidSetNumber;
}

namespace {
struct ReflectionContext {
    uint32_t currentSet = kInvalidSetNumber;
    uint32_t bindingOffset = 0;
    std::string prefix = "";
    sa::ShaderVisibility visibility = sa::ShaderVisibility::None;

    ReflectionContext WithPrefix(const std::string& name) const {
        return {currentSet, bindingOffset, prefix.empty() ? name : prefix + "." + name, visibility};
    }
    ReflectionContext WithSet(uint32_t newSet) const {
        return {newSet, bindingOffset, prefix, visibility};
    }
    ReflectionContext WithOffset(uint32_t offset) const {
        return {currentSet, bindingOffset + offset, prefix, visibility};
    }
};

sa::Texture CreateTextureBinding(VariableLayoutReflection* varLayout,
                                 const ReflectionContext& ctx) {
    slang::TypeLayoutReflection* typeLayout = varLayout->getTypeLayout();
    slang::TypeReflection* type = typeLayout->getType();

    sa::Texture textureBinding{};
    SlangResourceShape shape = varLayout->getTypeLayout()->getType()->getResourceShape();
    switch (shape) {
        case SLANG_TEXTURE_1D:
            textureBinding.viewDimension = sa::ShaderAssetFormat::ViewDimension::e1D;
            break;
        case SLANG_TEXTURE_2D:
            textureBinding.viewDimension = sa::ShaderAssetFormat::ViewDimension::e2D;
            break;
        case SLANG_TEXTURE_2D_ARRAY:
            textureBinding.viewDimension = sa::ShaderAssetFormat::ViewDimension::e2DArray;
            break;
        case SLANG_TEXTURE_CUBE:
            textureBinding.viewDimension = sa::ShaderAssetFormat::ViewDimension::Cube;
            break;
        case SLANG_TEXTURE_CUBE_ARRAY:
            textureBinding.viewDimension = sa::ShaderAssetFormat::ViewDimension::CubeArray;
            break;
        case SLANG_TEXTURE_3D:
            textureBinding.viewDimension = sa::ShaderAssetFormat::ViewDimension::e3D;
            break;
        case SLANG_TEXTURE_2D_MULTISAMPLE:
            textureBinding.viewDimension = sa::ShaderAssetFormat::ViewDimension::e2D;
            textureBinding.multiSampled = 1;
            break;
        case SLANG_TEXTURE_2D_MULTISAMPLE_ARRAY:
            textureBinding.viewDimension = sa::ShaderAssetFormat::ViewDimension::e2DArray;
            textureBinding.multiSampled = 1;
            break;
        default:
            textureBinding.viewDimension = sa::ShaderAssetFormat::ViewDimension::Undefined;
            break;
    }
    TypeReflection* resultType = type->getResourceResultType();
    TypeReflection::ScalarType scalarType = resultType->getScalarType();

    switch (scalarType) {
        case TypeReflection::ScalarType::Float16:
        case TypeReflection::ScalarType::Float32:
        case TypeReflection::ScalarType::Float64:
            textureBinding.type = sa::ShaderAssetFormat::TextureType::Float;
            break;
        case TypeReflection::ScalarType::UInt16:
        case TypeReflection::ScalarType::UInt32:
        case TypeReflection::ScalarType::UInt64:
            textureBinding.type = sa::ShaderAssetFormat::TextureType::Uint;
            break;
        case TypeReflection::ScalarType::Int16:
        case TypeReflection::ScalarType::Int32:
        case TypeReflection::ScalarType::Int64:
            textureBinding.type = sa::ShaderAssetFormat::TextureType::Sint;
            break;
        case TypeReflection::ScalarType::Bool:
            textureBinding.type = sa::ShaderAssetFormat::TextureType::Bool;
            break;
        default:
            break;
    }

    const char* typeName = type->getName();
    std::string_view typeNameView(typeName ? typeName : "");
    if (typeNameView.contains("DepthTexture")) {
        textureBinding.type = sa::ShaderAssetFormat::TextureType::Depth;
    }

    return textureBinding;
}

sa::Sampler CreateSamplerBinding(VariableLayoutReflection* varLayout,
                                 const ReflectionContext& ctx) {
    sa::Sampler samplerBinding{};
    // Slang의 SamplerState는 Filtering, NonFiltering, Comparison 등의 세부 타입을 가질 수 있음
    // 여기서는 단순히 Filtering 타입으로 매핑
    samplerBinding.type = sa::ShaderAssetFormat::SamplerType::Filtering;
    return samplerBinding;
}

std::optional<sa::Binding> CreateLeafBinding(VariableLayoutReflection* varLayout,
                                             const ReflectionContext& ctx) {
    TypeLayoutReflection* typeLayout = varLayout->getTypeLayout();
    TypeReflection::Kind kind = typeLayout->getKind();

    if (varLayout->getBindingIndex() == -1) {
        return std::nullopt;
    }

    uint32_t set = ctx.currentSet;
    if (set == -1) {
        size_t spaceOffset = varLayout->getOffset(slang::ParameterCategory::RegisterSpace);
        set = (spaceOffset != -SLANG_UNBOUNDED_SIZE) ? (uint32_t)spaceOffset : 0;
    }

    sa::Binding binding{};

    binding.set = set;
    binding.binding = ctx.bindingOffset + varLayout->getBindingIndex();

    switch (kind) {
        case slang::TypeReflection::Kind::ParameterBlock: {
            size_t cbufferIndex =
                varLayout->getOffset(slang::ParameterCategory::DescriptorTableSlot);
            if (cbufferIndex == SLANG_UNBOUNDED_SIZE) {
                cbufferIndex = 0;
                break;
            }
            binding.binding = static_cast<uint32_t>(cbufferIndex);
            [[fallthrough]];
        }
        case slang::TypeReflection::Kind::ConstantBuffer: {
            TypeLayoutReflection* innerType = typeLayout->getElementTypeLayout();
            if (innerType->getSize() > 0) {
                binding.resource.buffer.bufferSize = static_cast<uint32_t>(innerType->getSize());
            }
            binding.resourceType = sa::ResourceType::UniformBuffer;
            break;
        }
        case slang::TypeReflection::Kind::TextureBuffer:
            binding.resourceType = sa::ResourceType::ReadOnlyStorage;
            break;
        case slang::TypeReflection::Kind::ShaderStorageBuffer:
            binding.resourceType = sa::ResourceType::StorageBuffer;
            break;
        case slang::TypeReflection::Kind::Resource: {
            SlangResourceShape shape = typeLayout->getResourceShape();
            switch (shape) {
                case SLANG_TEXTURE_1D:
                case SLANG_TEXTURE_1D_ARRAY:
                case SLANG_TEXTURE_2D:
                case SLANG_TEXTURE_CUBE:
                case SLANG_TEXTURE_CUBE_ARRAY:
                case SLANG_TEXTURE_2D_ARRAY:
                case SLANG_TEXTURE_3D:
                    binding.resourceType = sa::ResourceType::Texture;
                    binding.resource = {.texture = CreateTextureBinding(varLayout, ctx)};
                    break;
                case SLANG_STRUCTURED_BUFFER:
                case SLANG_BYTE_ADDRESS_BUFFER: {
                    SlangResourceAccess access = typeLayout->getResourceAccess();
                    binding.resourceType = (access == SLANG_RESOURCE_ACCESS_READ)
                                               ? sa::ResourceType::ReadOnlyStorage
                                               : sa::ResourceType::StorageBuffer;
                    break;
                }
            }
            break;
        }
        case slang::TypeReflection::Kind::SamplerState:
            binding.resourceType = sa::ResourceType::Sampler;
            binding.resource = {.sampler = CreateSamplerBinding(varLayout, ctx)};
            break;
        default:
            binding.resourceType = sa::ResourceType::Unknown;
            break;
    }

    binding.visibility = ctx.visibility;
    if (!ctx.prefix.empty()) {
        std::string_view nameView = ctx.prefix;
        size_t copyLen = std::min(nameView.size(), sizeof(binding.name) - 1);
        std::memcpy(binding.name, nameView.data(), copyLen);
        binding.name[copyLen] = '\0';
    }
    return binding;
}

}  // namespace
std::vector<sa::Binding> ReflectRecursively(VariableLayoutReflection* varLayout,
                                            const ReflectionContext& ctx) {
    std::vector<sa::Binding> results;

    TypeLayoutReflection* typeLayout = varLayout->getTypeLayout();
    TypeReflection::Kind kind = typeLayout->getKind();

    const char* rawName = varLayout->getName();
    std::string varName = rawName == nullptr ? "" : std::string(rawName);

    if (kind == TypeReflection::Kind::ParameterBlock) {
        TypeLayoutReflection* innerType = typeLayout->getElementTypeLayout();

        uint32_t newSet = varLayout->getBindingSpace();
        size_t spaceOffset =
            varLayout->getOffset(slang::ParameterCategory::SubElementRegisterSpace);
        if (spaceOffset != SLANG_UNBOUNDED_SIZE) {
            newSet = static_cast<uint32_t>(spaceOffset);
        }

        ReflectionContext nexCtx = ctx.WithSet(newSet).WithPrefix(varName);
        uint32_t fieldCount = innerType->getFieldCount();
        for (uint32_t i = 0; i < fieldCount; ++i) {
            VariableLayoutReflection* fieldVar = innerType->getFieldByIndex(i);
            auto childBindings = ReflectRecursively(fieldVar, nexCtx);
            results.insert(results.end(), childBindings.begin(), childBindings.end());
        }

        auto childBindings = CreateLeafBinding(varLayout, nexCtx);
        if (childBindings) {
            results.push_back(childBindings.value());
        }

        return results;

    } else if (kind == TypeReflection::Kind::Struct) {
        uint32_t addedOffset = 0;
        if (varLayout->getBindingIndex() != -1) {
            addedOffset += varLayout->getBindingIndex();
        }

        ReflectionContext nexCtx = ctx.WithPrefix(varName).WithOffset(addedOffset);

        uint32_t fieldCount = typeLayout->getFieldCount();
        for (uint32_t i = 0; i < fieldCount; ++i) {
            VariableLayoutReflection* fieldRef = typeLayout->getFieldByIndex(i);
            auto childBindings = ReflectRecursively(fieldRef, nexCtx);
            results.insert(results.end(), childBindings.begin(), childBindings.end());
        }
        return results;
    }

    ReflectionContext leafCtx = ctx.WithPrefix(varName);
    if (auto binding = CreateLeafBinding(varLayout, leafCtx)) {
        results.push_back(*binding);
    }

    return results;
}

std::vector<sa::Binding> GenerateBindingsFromLayout(slang::ProgramLayout* layout,
                                                    sa::ShaderVisibility visibility) {
    std::vector<sa::Binding> bindings;
    uint32_t paramCount = layout->getParameterCount();
    ReflectionContext ctx{
        .visibility = visibility,
    };
    for (uint32_t i = 0; i < paramCount; ++i) {
        VariableLayoutReflection* varRefl = layout->getParameterByIndex(i);
        std::string empty("");
        // Search recursively binding info from slang reflection tree and store in the binsings.

        auto result = ReflectRecursively(varRefl, ctx);
        bindings.insert(bindings.end(), result.begin(), result.end());
    }

    std::ranges::sort(bindings, [](sa::Binding& a, sa::Binding& b) -> bool {
        if (a.set != b.set) {
            return a.set < b.set;
        }
        return a.binding < b.binding;
    });

    return bindings;
}

static sa::ShaderVisibility GetVisibility(SlangStage stage) {
    switch (stage) {
        case SLANG_STAGE_VERTEX:
            return sa::ShaderVisibility::Vertex;
        case SLANG_STAGE_FRAGMENT:
            return sa::ShaderVisibility::Fragment;
        case SLANG_STAGE_COMPUTE:
            return sa::ShaderVisibility::Compute;
        default:
            return sa::ShaderVisibility::None;
    }
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
    sa::ShaderVisibility visibility = sa::ShaderVisibility::None;
    if (layout->getEntryPointCount() > 0) {
        visibility = GetVisibility(layout->getEntryPointByIndex(0)->getStage());
    }

    std::vector<sa::Binding> bindings = GenerateBindingsFromLayout(layout, visibility);

    std::vector<uint8_t> code(codeBlob->getBufferSize());
    memcpy(code.data(), codeBlob->getBufferPointer(), codeBlob->getBufferSize());
    sa::Header header = {
        .bindingCount = static_cast<uint16_t>(bindings.size()),
        .shaderSize = static_cast<uint32_t>(code.size()),
        .entryShaderStage = visibility,
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
        options.emplace_back(
            slang::CompilerOptionEntry{.name = slang::CompilerOptionName::MatrixLayoutColumn,
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