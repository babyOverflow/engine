
#include <algorithm>
#include <filesystem>
#include <print>
#include <ranges>

#include <slang-com-helper.h>
#include <slang-com-ptr.h>
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

std::expected<CompileResult, Error> SlangCompiler::CompileFromString(const std::string& slangCode,
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

std::expected<CompileResult, Error> SlangCompiler::CompileFromString(const std::string& slangCode) {
    CompilationContext context;
    ComPtr<IBlob> diagnosticBlob;

    ComPtr<ISession> session;
    if (auto result = CreateSession(); result.has_value()) {
        session = result.value();
    } else {
        return std::unexpected(result.error());
    }

    std::vector<ComPtr<slang::IComponentType>> componentsToLink;
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
        componentsToLink.push_back(ComPtr<slang::IComponentType>(module.get()));
    }

    int definedEntryPointCount = module->getDefinedEntryPointCount();
    for (int i = 0; i < definedEntryPointCount; i++) {
        ComPtr<slang::IEntryPoint> entryPoint;
        if (SLANG_FAILED(module->getDefinedEntryPoint(i, entryPoint.writeRef()))) {
            // TODO!
        }
        componentsToLink.push_back(ComPtr<slang::IComponentType>(entryPoint.get()));
    }

    ComPtr<IComponentType> composedProgram;
    {
        const auto result = session->createCompositeComponentType(
            reinterpret_cast<slang::IComponentType**>(componentsToLink.data()),
            componentsToLink.size(), composedProgram.writeRef(), diagnosticBlob.writeRef());

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
    bool skipResourceDetails = false;

    ReflectionContext WithPrefix(const std::string& name) const {
        return {currentSet, bindingOffset, prefix.empty() ? name : prefix + "." + name, visibility,
                skipResourceDetails};
    }
    ReflectionContext WithSet(uint32_t newSet) const {
        return {newSet, bindingOffset, prefix, visibility, skipResourceDetails};
    }
    ReflectionContext WithOffset(uint32_t offset) const {
        return {currentSet, bindingOffset + offset, prefix, visibility, skipResourceDetails};
    }
};

struct VaryingParameterContext {
    std::string prefix = "";

    VaryingParameterContext WithPrefix(const std::string& name) const {
        return {prefix.empty() ? name : prefix + "." + name};
    }
};

struct CompilerBinding {
    uint32_t set;
    uint32_t binding;
    uint32_t id;
    sa::Resource resource = {.buffer = {0}};
    sa::ResourceType resourceType;
    sa::ShaderVisibility visibility;
    std::string name;

    slang::ParameterCategory slangCategory;
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

    samplerBinding.type = sa::ShaderAssetFormat::SamplerType::Filtering;
    return samplerBinding;
}

CompilerBinding CalculateBasicBinding(VariableLayoutReflection* varLayout,
                                      const ReflectionContext& ctx,
                                      slang::TypeReflection::Kind kind) {
    CompilerBinding binding{};

    uint32_t set = ctx.currentSet;
    if (set == static_cast<uint32_t>(-1)) {  // kInvalidSetNumber
        size_t spaceOffset = varLayout->getOffset(slang::ParameterCategory::RegisterSpace);
        set = (spaceOffset != -SLANG_UNBOUNDED_SIZE) ? static_cast<uint32_t>(spaceOffset) : 0;
    }

    binding.set = set;
    binding.binding = ctx.bindingOffset + varLayout->getBindingIndex();
    binding.id = core::ToPropertyID(varLayout->getName());

    if (kind == slang::TypeReflection::Kind::ParameterBlock) {
        size_t cbufferIndex = varLayout->getOffset(slang::ParameterCategory::DescriptorTableSlot);
        if (cbufferIndex != SLANG_UNBOUNDED_SIZE) {
            binding.binding = static_cast<uint32_t>(cbufferIndex);
        }
    }
    binding.slangCategory = varLayout->getCategory();

    return binding;
}

bool PopulateResourceDetails(CompilerBinding& binding,
                             VariableLayoutReflection* varLayout,
                             const ReflectionContext& ctx,
                             slang::TypeReflection::Kind kind) {
    TypeLayoutReflection* typeLayout = varLayout->getTypeLayout();

    switch (kind) {
        case slang::TypeReflection::Kind::ParameterBlock:
            [[fallthrough]];
        case slang::TypeReflection::Kind::ConstantBuffer: {
            TypeLayoutReflection* innerType = typeLayout->getElementTypeLayout();
            if (innerType->getSize() == 0) {
                return false;
            }
            binding.resource.buffer.bufferSize = static_cast<uint32_t>(innerType->getSize());
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
            return false;
    }

    binding.visibility = ctx.visibility;
    binding.name = ctx.prefix;
    return true;
}

std::optional<CompilerBinding> CreateLeafBinding(VariableLayoutReflection* varLayout,
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

    CompilerBinding binding = CalculateBasicBinding(varLayout, ctx, kind);

    if (kind == slang::TypeReflection::Kind::ParameterBlock) {
        size_t cbufferIndex = varLayout->getOffset(slang::ParameterCategory::DescriptorTableSlot);
        if (cbufferIndex != SLANG_UNBOUNDED_SIZE) {
            binding.binding = static_cast<uint32_t>(cbufferIndex);
        }
    }

    if (ctx.skipResourceDetails) {
        return binding;
    }

    if (!PopulateResourceDetails(binding, varLayout, ctx, kind)) {
        return std::nullopt;  
    }

    return binding;
}

}  // namespace
std::vector<CompilerBinding> ReflectRecursively(VariableLayoutReflection* varLayout,
                                                const ReflectionContext& ctx) {
    std::vector<CompilerBinding> results;

    TypeLayoutReflection* typeLayout = varLayout->getTypeLayout();
    TypeReflection::Kind kind = typeLayout->getKind();

    std::string varName = "";
    if (!ctx.skipResourceDetails) {
        const char* rawName = varLayout->getName();
        std::string varName = rawName == nullptr ? "" : std::string(rawName);
    }

    if (kind == TypeReflection::Kind::ParameterBlock) {
        TypeLayoutReflection* innerType = typeLayout->getElementTypeLayout();

        uint32_t newSet = varLayout->getBindingSpace();
        size_t spaceOffset =
            varLayout->getOffset(slang::ParameterCategory::SubElementRegisterSpace);
        if (spaceOffset != SLANG_UNBOUNDED_SIZE) {
            newSet = static_cast<uint32_t>(spaceOffset);
        }

        ReflectionContext nextCtx = ctx.WithSet(newSet).WithPrefix(varName);
        uint32_t fieldCount = innerType->getFieldCount();
        for (uint32_t i = 0; i < fieldCount; ++i) {
            VariableLayoutReflection* fieldVar = innerType->getFieldByIndex(i);
            auto childBindings = ReflectRecursively(fieldVar, nextCtx);
            results.insert(results.end(), childBindings.begin(), childBindings.end());
        }

        auto childBindings = CreateLeafBinding(varLayout, nextCtx);
        if (childBindings) {
            results.push_back(childBindings.value());
        }

        return results;

    } else if (kind == TypeReflection::Kind::Struct) {
        uint32_t addedOffset = 0;
        if (varLayout->getBindingIndex() != -1) {
            addedOffset += varLayout->getBindingIndex();
        }

        ReflectionContext nextCtx = ctx.WithPrefix(varName).WithOffset(addedOffset);

        uint32_t fieldCount = typeLayout->getFieldCount();
        for (uint32_t i = 0; i < fieldCount; ++i) {
            VariableLayoutReflection* fieldRef = typeLayout->getFieldByIndex(i);
            auto childBindings = ReflectRecursively(fieldRef, nextCtx);
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

std::vector<CompilerBinding> GenerateBindingsFromLayout(slang::ProgramLayout* layout,
                                                        CompilationContext& compilationContext) {
    std::vector<CompilerBinding> bindings;
    uint32_t paramCount = layout->getParameterCount();
    ReflectionContext ctx{
        .visibility = compilationContext.visibility,
    };
    for (uint32_t i = 0; i < paramCount; ++i) {
        VariableLayoutReflection* varRefl = layout->getParameterByIndex(i);
        std::string empty("");
        // Search recursively binding info from slang reflection tree and store in the binsings.
        size_t spaceOffset = varRefl->getBindingSpace();
        const std::vector<CompilerBinding> result =
            ReflectRecursively(varRefl, ctx.WithSet(spaceOffset));
        bindings.insert(bindings.end(), result.begin(), result.end());
    }

    std::ranges::sort(bindings, [](CompilerBinding& a, CompilerBinding& b) -> bool {
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
namespace {
struct CompilerVariable {
    sa::Kind kind;
    sa::ScalarType scalarType;
    sa::Shape shape;
    std::string name;
};
struct CompilerShaderParameter {
    CompilerVariable variable;
    uint32_t location;
    std::optional<std::string> semanticName;
};
struct CompilerEntryParameter {
    std::vector<CompilerShaderParameter> input;
    std::vector<CompilerShaderParameter> output;
};
}  // namespace

std::vector<CompilerShaderParameter> CreateVaryingParameterLayout(
    slang::VariableLayoutReflection* varLayout,
    const VaryingParameterContext& context) {
    slang::TypeLayoutReflection* typeLayout = varLayout->getTypeLayout();
    std::vector<CompilerShaderParameter> shaderParameters;
    CompilerVariable variable{};
    switch (typeLayout->getKind()) {
        case slang::TypeReflection::Kind::Scalar: {
            variable.kind = sa::Kind::Scalar;
            variable.shape = sa::Shape::Scalar();
            break;
        }
        case slang::TypeReflection::Kind::Vector: {
            variable.kind = sa::Kind::Vector;
            variable.shape = sa::Shape::Vector(typeLayout->getElementCount());
            break;
        }
        case slang::TypeReflection::Kind::Matrix: {
            variable.kind = sa::Kind::Matrix;
            variable.shape =
                sa::Shape::Matrix(typeLayout->getColumnCount(), typeLayout->getRowCount());
            break;
        }
        case slang::TypeReflection::Kind::Struct: {
            uint32_t fieldCount = typeLayout->getFieldCount();
            for (uint32_t i = 0; i < fieldCount; ++i) {
                auto* param = typeLayout->getFieldByIndex(i);
                const VaryingParameterContext nextCtx = context.WithPrefix(param->getName());
                std::vector<CompilerShaderParameter> fields =
                    CreateVaryingParameterLayout(param, nextCtx);
                shaderParameters.append_range(fields);
                continue;
            }
            return shaderParameters;
        }
        default:
            break;
    }
    slang::TypeReflection::ScalarType scalarType = typeLayout->getScalarType();
    switch (scalarType) {
        case slang::TypeReflection::ScalarType::Float16:
            variable.scalarType = sa::ScalarType::F16;
            break;
        case slang::TypeReflection::ScalarType::Float32:
            variable.scalarType = sa::ScalarType::F32;
            break;
        case slang::TypeReflection::ScalarType::UInt32:
            variable.scalarType = sa::ScalarType::U32;
            break;
        case slang::TypeReflection::ScalarType::Int32:
            variable.scalarType = sa::ScalarType::I32;
            break;
        case slang::TypeReflection::ScalarType::Bool:
            variable.scalarType = sa::ScalarType::Bool;
            break;
        case slang::TypeReflection::ScalarType::Int16:
        case slang::TypeReflection::ScalarType::Int64:
        case slang::TypeReflection::ScalarType::UInt16:
        case slang::TypeReflection::ScalarType::UInt64:
        case slang::TypeReflection::ScalarType::Float64:
        default:
            break;
    }

    variable.name = context.prefix;

    const char* semanticName = varLayout->getSemanticName();
    const uint32_t location = varLayout->getOffset(slang::ParameterCategory::VaryingInput);
    // const uint32_t location = varLayout->getBindingIndex();
    shaderParameters.push_back(CompilerShaderParameter{
        .variable = variable,
        .location = location,
        .semanticName = semanticName == nullptr ? std::nullopt : std::make_optional(semanticName),
    });
    return shaderParameters;
}

CompilerEntryParameter GernerateEntryPointLayout(slang::EntryPointReflection* layout,
                                                 CompilationContext& context) {
    VaryingParameterContext varyingContext;
    slang::VariableLayoutReflection* paramaterVarLayout = layout->getVarLayout();
    std::vector<CompilerShaderParameter> entryInputParameters =
        CreateVaryingParameterLayout(paramaterVarLayout, varyingContext);
    slang::VariableLayoutReflection* resultVarLayout = layout->getResultVarLayout();
    // const uint32_t resultVarCount
    std::vector<CompilerShaderParameter> entryOutpuParameters =
        CreateVaryingParameterLayout(resultVarLayout, varyingContext);
    return {
        entryInputParameters,
        entryOutpuParameters,
    };
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

    std::vector<std::string> nameTable;
    std::unordered_map<std::string, uint32_t> stringPool;
    const auto getNameId = [&](const std::string& name) {
        const auto [it, inserted] = stringPool.try_emplace(name, nameTable.size());
        if (inserted) {
            nameTable.push_back(name);
        }
        return it->second;
    };

    std::vector<sa::ShaderParameter> parameters;
    const auto attachParameter = [&](const CompilerShaderParameter& cp) {
        const auto& variable = cp.variable;
        sa::ShaderParameter parameter{
            .variable =
                sa::Variable{
                    .kind = variable.kind,
                    .scalarType = variable.scalarType,
                    .shape = variable.shape,
                    .nameIdx = getNameId(variable.name),
                },
            .location = cp.location,
            .semanticNameIdx =
                cp.semanticName.has_value() ? getNameId(cp.semanticName.value()) : sa::kInvalidIdx,
        };

        const auto it = std::ranges::find(parameters, parameter);
        if (it == parameters.end()) {
            parameters.push_back(parameter);
            return parameters.size() - 1;
        } else {
            return static_cast<std::size_t>(std::distance(parameters.begin(), it));
        }
    };

    ProgramLayout* programLayout = linkedProgram->getLayout();
    std::vector<CompilerBinding> compilerBindings =
        GenerateBindingsFromLayout(programLayout, context);

    uint32_t entryCount = programLayout->getEntryPointCount();
    std::vector<uint32_t> indices;
    std::vector<sa::EntryPoint> entryPoints;
    for (uint32_t i = 0; i < entryCount; ++i) {
        slang::EntryPointReflection* entry = programLayout->getEntryPointByIndex(i);
        sa::ShaderVisibility visibility = GetVisibility(entry->getStage());
        context.visibility = visibility;
        CompilerEntryParameter compilerParameters = GernerateEntryPointLayout(entry, context);
        sa::EntryPoint entryPoint;

        entryPoint.stage = context.visibility;
        if ((entryPoint.stage & sa::ShaderVisibility::Vertex) == sa::ShaderVisibility::Vertex) {
            entryPoint.ioStartIndex = indices.size();
            entryPoint.ioCount = compilerParameters.input.size();
            for (uint32_t i = 0; i < compilerParameters.input.size(); ++i) {
                uint32_t idx = attachParameter(compilerParameters.input[i]);
                indices.push_back(idx);
            }
        }
        if ((entryPoint.stage & sa::ShaderVisibility::Fragment) == sa::ShaderVisibility::Fragment) {
            entryPoint.ioStartIndex = indices.size();
            entryPoint.ioCount = compilerParameters.output.size();
            for (uint32_t i = 0; i < compilerParameters.output.size(); ++i) {
                uint32_t idx = attachParameter(compilerParameters.output[i]);
                indices.push_back(idx);
            }
        }

        entryPoint.bindingStartIndex = indices.size();
        entryPoint.bindingCount = 0;
        ComPtr<IMetadata> metadata;
        composedProgram->getEntryPointMetadata(i, 0, metadata.writeRef());
        for (uint32_t j = 0; j < compilerBindings.size(); ++j) {
            const auto& b = compilerBindings[j];
            bool isUsed = false;

            metadata->isParameterLocationUsed(static_cast<SlangParameterCategory>(b.slangCategory),
                                              b.set, b.binding, isUsed);

            if (isUsed) {
                entryPoint.bindingCount++;
                indices.push_back(j);
                compilerBindings[j].visibility = compilerBindings[j].visibility | entryPoint.stage;
            }
        }
        uint32_t nameIdx = getNameId(entry->getName());
        entryPoint.nameIdx = nameIdx;

        entryPoints.push_back(entryPoint);
    }

    std::vector<sa::Binding> bindings =
        compilerBindings | std::views::transform([&](CompilerBinding& cb) -> sa::Binding {
            return sa::Binding{
                .set = cb.set,
                .binding = cb.binding,
                .id = cb.id,
                .nameIdx = getNameId(cb.name),
                .resource = cb.resource,
                .resourceType = cb.resourceType,
                .visibility = cb.visibility,
            };
        }) |
        std::ranges::to<std::vector>();

    uint32_t nameTableSize = std::ranges::fold_left(
        nameTable, 0u, [](uint32_t acc, const std::string& name) { return acc + name.size() + 1; });

    std::vector<uint8_t> code(codeBlob->getBufferSize());
    std::memcpy(code.data(), codeBlob->getBufferPointer(), codeBlob->getBufferSize());

    // temp return
    return std::expected<CompileResult, Error>(CompileResult{.parameters = std::move(parameters),
                                                             .bindings = std::move(bindings),
                                                             .entryPoints = std::move(entryPoints),
                                                             .sourceBlob = std::move(code),
                                                             .nameTable = std::move(nameTable),
                                                             .indices = std::move(indices),
                                                             .warning = context.message});
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
                                           .intValue0 = 1 
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