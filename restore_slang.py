import codecs

content = r'''#include <algorithm>
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
        SlangGlobalSessionDesc desc{};
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

constexpr uint32_t kInvalidSetNumber = -1;

namespace {
struct ReflectionContext {
    uint32_t currentSet = kInvalidSetNumber;
    std::string prefix = "";
    sa::ShaderVisibility visibility = sa::ShaderVisibility::None;
    IMetadata* metadata = nullptr;

    ReflectionContext WithPrefix(const std::string& name) const {
        return {currentSet, prefix.empty() ? name : prefix + "." + name, visibility, metadata};
    }
    ReflectionContext WithSet(uint32_t newSet) const {
        return {newSet, prefix, visibility, metadata};
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
};

static sa::ShaderVisibility GetVisibility(SlangStage stage) {
    switch (stage) {
        case SLANG_STAGE_VERTEX: return sa::ShaderVisibility::Vertex;
        case SLANG_STAGE_FRAGMENT: return sa::ShaderVisibility::Fragment;
        case SLANG_STAGE_COMPUTE: return sa::ShaderVisibility::Compute;
        default: return sa::ShaderVisibility::None;
    }
}

sa::Texture CreateTextureBinding(VariableLayoutReflection* varLayout) {
    sa::Texture textureBinding{};
    TypeLayoutReflection* typeLayout = varLayout->getTypeLayout();
    SlangResourceShape shape = typeLayout->getType()->getResourceShape();
    
    switch (shape) {
        case SLANG_TEXTURE_1D: textureBinding.viewDimension = sa::ShaderAssetFormat::ViewDimension::e1D; break;
        case SLANG_TEXTURE_2D: textureBinding.viewDimension = sa::ShaderAssetFormat::ViewDimension::e2D; break;
        case SLANG_TEXTURE_CUBE: textureBinding.viewDimension = sa::ShaderAssetFormat::ViewDimension::Cube; break;
        case SLANG_TEXTURE_3D: textureBinding.viewDimension = sa::ShaderAssetFormat::ViewDimension::e3D; break;
        default: break;
    }

    TypeReflection* resultType = typeLayout->getType()->getResourceResultType();
    if (resultType) {
        auto scalar = resultType->getScalarType();
        if (scalar == TypeReflection::ScalarType::Float32) textureBinding.type = sa::ShaderAssetFormat::TextureType::Float;
    }
    return textureBinding;
}

void ReflectEntryBindings(VariableLayoutReflection* varLayout, 
                          const ReflectionContext& ctx, 
                          std::vector<CompilerBinding>& outBindings) {
    
    TypeLayoutReflection* typeLayout = varLayout->getTypeLayout();
    TypeReflection::Kind kind = typeLayout->getKind();
    const char* rawName = varLayout->getName();
    std::string currentVarName = rawName ? rawName : "";

    if (ctx.metadata) {
        bool isUsed = false;
        ParameterCategory cat = varLayout->getCategory();
        if (static_cast<SlangParameterCategory>(cat) != SLANG_PARAMETER_CATEGORY_MIXED) {
            ctx.metadata->isParameterLocationUsed(static_cast<SlangParameterCategory>(cat), 
                                              varLayout->getBindingSpace(), 
                                              varLayout->getBindingIndex(), 
                                              isUsed);
            if (!isUsed && kind != TypeReflection::Kind::ParameterBlock && kind != TypeReflection::Kind::Struct) {
                return;
            }
        }
    }

    if (kind == TypeReflection::Kind::ParameterBlock) {
        uint32_t blockSet = varLayout->getBindingSpace();
        
        size_t subSpaceOffset = varLayout->getOffset(ParameterCategory::SubElementRegisterSpace);
        if (subSpaceOffset != SLANG_UNBOUNDED_SIZE) {
            blockSet = static_cast<uint32_t>(subSpaceOffset);
        }

        ReflectionContext nextCtx = ctx.WithSet(blockSet).WithPrefix(currentVarName);
        
        TypeLayoutReflection* innerType = typeLayout->getElementTypeLayout();
        for (uint32_t i = 0; i < innerType->getFieldCount(); ++i) {
            ReflectEntryBindings(innerType->getFieldByIndex(i), nextCtx, outBindings);
        }
        
        if (varLayout->getBindingIndex() != -1) {
            CompilerBinding b{};
            b.set = blockSet;
            b.binding = varLayout->getBindingIndex();
            b.resourceType = sa::ResourceType::UniformBuffer;
            b.resource.buffer.bufferSize = (uint32_t)innerType->getSize();
            b.name = nextCtx.prefix;
            b.visibility = ctx.visibility;
            outBindings.push_back(b);
        }
    } 
    else if (kind == TypeReflection::Kind::Struct) {
        ReflectionContext nextCtx = ctx.WithPrefix(currentVarName);
        for (uint32_t i = 0; i < typeLayout->getFieldCount(); ++i) {
            ReflectEntryBindings(typeLayout->getFieldByIndex(i), nextCtx, outBindings);
        }
    } 
    else {
        if (varLayout->getBindingIndex() == -1) return;

        CompilerBinding b{};
        uint32_t set = ctx.currentSet;
        if (set == kInvalidSetNumber) {
            set = varLayout->getBindingSpace();
        }
        b.set = set;
        b.binding = (uint32_t)varLayout->getBindingIndex();
        b.visibility = ctx.visibility;
        b.name = ctx.WithPrefix(currentVarName).prefix;

        switch (kind) {
            case TypeReflection::Kind::ConstantBuffer:
                b.resourceType = sa::ResourceType::UniformBuffer;
                b.resource.buffer.bufferSize = (uint32_t)typeLayout->getElementTypeLayout()->getSize();
                break;
            case TypeReflection::Kind::Resource:
                b.resourceType = sa::ResourceType::Texture;
                b.resource.texture = CreateTextureBinding(varLayout);
                break;
            case TypeReflection::Kind::SamplerState:
                b.resourceType = sa::ResourceType::Sampler;
                break;
            default: return;
        }
        outBindings.push_back(b);
    }
}

struct CompilerShaderParameter {
    sa::Variable var;
    uint32_t location;
    core::Semantic semantic;
};

void ReflectVarying(VariableLayoutReflection* varLayout, 
                    const std::string& prefix, 
                    std::vector<CompilerShaderParameter>& outParams,
                    std::function<uint32_t(const std::string&)> getNameId) {
    
    TypeLayoutReflection* typeLayout = varLayout->getTypeLayout();
    if (typeLayout->getKind() == TypeReflection::Kind::Struct) {
        for (uint32_t i = 0; i < typeLayout->getFieldCount(); ++i) {
            auto field = typeLayout->getFieldByIndex(i);
            std::string name = prefix.empty() ? field->getName() : prefix + "." + field->getName();
            ReflectVarying(field, name, outParams, getNameId);
        }
        return;
    }

    CompilerShaderParameter p{};
    p.location = (uint32_t)varLayout->getOffset(ParameterCategory::VaryingInput);
    if (p.location == (uint32_t)SLANG_UNBOUNDED_SIZE) p.location = 0;

    p.var.nameIdx = getNameId(prefix);
    
    switch (typeLayout->getKind()) {
        case TypeReflection::Kind::Scalar: p.var.kind = sa::Kind::Scalar; p.var.shape = sa::Shape::Scalar(); break;
        case TypeReflection::Kind::Vector: p.var.kind = sa::Kind::Vector; p.var.shape = sa::Shape::Vector(typeLayout->getElementCount()); break;
        case TypeReflection::Kind::Matrix: p.var.kind = sa::Kind::Matrix; p.var.shape = sa::Shape::Matrix(typeLayout->getColumnCount(), typeLayout->getRowCount()); break;
        default: p.var.kind = sa::Kind::None; break;
    }

    switch (typeLayout->getScalarType()) {
        case TypeReflection::ScalarType::Float32: p.var.scalarType = sa::ScalarType::F32; break;
        case TypeReflection::ScalarType::Float16: p.var.scalarType = sa::ScalarType::F16; break;
        case TypeReflection::ScalarType::UInt32: p.var.scalarType = sa::ScalarType::U32; break;
        case TypeReflection::ScalarType::Int32: p.var.scalarType = sa::ScalarType::I32; break;
        case TypeReflection::ScalarType::Bool: p.var.scalarType = sa::ScalarType::Bool; break;
        default: p.var.scalarType = sa::ScalarType::Undefined; break;
    }

    const char* sem = varLayout->getSemanticName();
    p.semantic = sem ? core::NameToSemantic(sem, (uint32_t)varLayout->getSemanticIndex()) : core::Semantic::Undefined;

    outParams.push_back(p);
}

} // namespace

std::expected<CompileResult, Error> slangCompiler::SlangCompiler::CompileInternal(
    slang::IComponentType* composedProgram,
    CompilationContext& context) {
    
    ComPtr<IComponentType> linkedProgram;
    ComPtr<IBlob> diagnosticBlob;
    composedProgram->link(linkedProgram.writeRef(), diagnosticBlob.writeRef());
    
    ComPtr<IBlob> codeBlob;
    linkedProgram->getTargetCode(0, codeBlob.writeRef(), diagnosticBlob.writeRef());

    ProgramLayout* layout = linkedProgram->getLayout();
    std::vector<std::string> nameTable;
    std::unordered_map<std::string, uint32_t> stringPool;
    const auto getNameId = [&](const std::string& name) {
        if (name.empty()) return 0u;
        auto [it, inserted] = stringPool.try_emplace(name, (uint32_t)nameTable.size());
        if (inserted) nameTable.push_back(name);
        return it->second;
    };

    std::vector<sa::ShaderParameter> parameters;
    std::vector<sa::Binding> bindings;
    std::vector<sa::EntryPoint> entryPoints;

    uint32_t entryCount = layout->getEntryPointCount();
    for (uint32_t i = 0; i < entryCount; ++i) {
        EntryPointReflection* entry = layout->getEntryPointByIndex(i);
        sa::ShaderVisibility visibility = GetVisibility(entry->getStage());
        
        ComPtr<IMetadata> metadata;
        linkedProgram->getEntryPointMetadata(i, 0, metadata.writeRef());

        sa::EntryPoint ep{};
        ep.stage = visibility;
        ep.nameIdx = getNameId(entry->getName());

        ep.bindingStartIndex = (uint32_t)bindings.size();
        std::vector<CompilerBinding> entryBinds;
        ReflectionContext refCtx{kInvalidSetNumber, "", visibility, metadata.get()};
        
        for (uint32_t j = 0; j < layout->getParameterCount(); ++j) {
            ReflectEntryBindings(layout->getParameterByIndex(j), refCtx, entryBinds);
        }

        std::ranges::sort(entryBinds, [](auto& a, auto& b) {
            if (a.set != b.set) return a.set < b.set;
            return a.binding < b.binding;
        });

        for (auto& b : entryBinds) {
            bindings.push_back(sa::Binding{
                .set = b.set, .binding = b.binding, .id = 0, .nameIdx = getNameId(b.name),
                .resource = b.resource, .resourceType = b.resourceType, .visibility = b.visibility
            });
        }
        ep.bindingCount = (uint16_t)(bindings.size() - ep.bindingStartIndex);

        ep.ioStartIndex = (uint32_t)parameters.size();
        std::vector<CompilerShaderParameter> varyingParams;
        if (visibility == sa::ShaderVisibility::Vertex) {
            ReflectVarying(entry->getVarLayout(), "", varyingParams, getNameId);
        } else if (visibility == sa::ShaderVisibility::Fragment) {
            ReflectVarying(entry->getResultVarLayout(), "", varyingParams, getNameId);
        }

        for (auto& vp : varyingParams) {
            parameters.push_back(sa::ShaderParameter{.variable = vp.var, .location = vp.location, .semantic = vp.semantic});
        }
        ep.ioCount = (uint16_t)(parameters.size() - ep.ioStartIndex);

        entryPoints.push_back(ep);
    }

    std::vector<uint8_t> code(codeBlob->getBufferSize());
    std::memcpy(code.data(), codeBlob->getBufferPointer(), codeBlob->getBufferSize());

    return CompileResult{
        .parameters = std::move(parameters),
        .bindings = std::move(bindings),
        .variables = {},
        .entryPoints = std::move(entryPoints),
        .sourceBlob = std::move(code),
        .nameTable = std::move(nameTable),
        .indices = {},
        .warning = context.message
    };
}

std::expected<CompileResult, Error> SlangCompiler::CompileFromString(const std::string& slangCode,
                                                                     const std::string& entryName) {
    CompilationContext context;
    ComPtr<IBlob> diagnosticBlob;
    ComPtr<ISession> session;
    if (auto result = CreateSession(); result.has_value()) session = result.value();
    else return std::unexpected(result.error());

    ComPtr<IModule> module;
    IModule* m = session->loadModuleFromSourceString("TempModule", "TempModule.slang", slangCode.data(), diagnosticBlob.writeRef());
    if (m == nullptr) {
        context.AppendError(diagnosticBlob.get());
        return std::unexpected(Error{ErrorType::InitFailed, "Load module failed: " + context.message});
    }
    module = m;

    ComPtr<IEntryPoint> entry;
    if (SLANG_FAILED(module->findEntryPointByName(entryName.data(), entry.writeRef()))) {
        return std::unexpected(Error{ErrorType::EntryPointNotFound, "Entry not found: " + entryName});
    }

    ComPtr<IComponentType> composedProgram;
    IComponentType* componentTypes[] = {module.get(), entry.get()};
    session->createCompositeComponentType(componentTypes, 2, composedProgram.writeRef(), diagnosticBlob.writeRef());

    return CompileInternal(composedProgram.get(), context);
}

std::expected<CompileResult, Error> SlangCompiler::CompileFromString(const std::string& slangCode) {
    CompilationContext context;
    ComPtr<IBlob> diagnosticBlob;
    ComPtr<ISession> session;
    if (auto result = CreateSession(); result.has_value()) session = result.value();
    else return std::unexpected(result.error());

    std::vector<ComPtr<slang::IComponentType>> componentsToLink;
    ComPtr<IModule> module;
    IModule* m = session->loadModuleFromSourceString("TempModule", "TempModule.slang", slangCode.data(), diagnosticBlob.writeRef());
    if (m == nullptr) {
        context.AppendError(diagnosticBlob.get());
        return std::unexpected(Error{ErrorType::InitFailed, "Load module failed: " + context.message});
    }
    module = m;
    componentsToLink.push_back(ComPtr<slang::IComponentType>(module.get()));

    int count = module->getDefinedEntryPointCount();
    for (int i = 0; i < count; i++) {
        ComPtr<slang::IEntryPoint> ep;
        module->getDefinedEntryPoint(i, ep.writeRef());
        componentsToLink.push_back(ComPtr<slang::IComponentType>(ep.get()));
    }

    ComPtr<IComponentType> composedProgram;
    session->createCompositeComponentType(reinterpret_cast<slang::IComponentType**>(componentsToLink.data()), (uint32_t)componentsToLink.size(), composedProgram.writeRef(), diagnosticBlob.writeRef());

    return CompileInternal(composedProgram.get(), context);
}

std::expected<CompileResult, Error> SlangCompiler::Compile(const std::string& path,
                                                           const std::string& entryName) {
    CompilationContext context;
    ComPtr<ISession> session;
    if (auto result = CreateSession(); result.has_value()) session = result.value();
    else return std::unexpected(result.error());

    ComPtr<IModule> module;
    ComPtr<IBlob> diagnosticBlob;
    IModule* m = session->loadModule(path.data(), diagnosticBlob.writeRef());
    if (m == nullptr) {
        context.AppendError(diagnosticBlob.get());
        return std::unexpected(Error{ErrorType::InitFailed, "Load module failed: " + context.message});
    }
    module = m;

    ComPtr<IEntryPoint> entry;
    if (SLANG_FAILED(module->findEntryPointByName(entryName.data(), entry.writeRef()))) {
        return std::unexpected(Error{ErrorType::EntryPointNotFound, "Entry not found: " + entryName});
    }

    ComPtr<IComponentType> composedProgram;
    IComponentType* componentTypes[] = {module.get(), entry.get()};
    session->createCompositeComponentType(componentTypes, 2, composedProgram.writeRef(), diagnosticBlob.writeRef());

    return CompileInternal(composedProgram.get(), context);
}

std::expected<CompileResult, Error> SlangCompiler::Compile(const std::string& path) {
    CompilationContext context;
    ComPtr<ISession> session;
    if (auto result = CreateSession(); result.has_value()) session = result.value();
    else return std::unexpected(result.error());

    std::vector<ComPtr<slang::IComponentType>> componentsToLink;
    ComPtr<IModule> module;
    ComPtr<IBlob> diagnosticBlob;
    IModule* m = session->loadModule(path.data(), diagnosticBlob.writeRef());
    if (m == nullptr) {
        context.AppendError(diagnosticBlob.get());
        return std::unexpected(Error{ErrorType::InitFailed, "Load module failed: " + context.message});
    }
    module = m;
    componentsToLink.push_back(ComPtr<slang::IComponentType>(module.get()));

    int count = module->getDefinedEntryPointCount();
    for (int i = 0; i < count; i++) {
        ComPtr<slang::IEntryPoint> ep;
        module->getDefinedEntryPoint(i, ep.writeRef());
        componentsToLink.push_back(ComPtr<slang::IComponentType>(ep.get()));
    }

    ComPtr<IComponentType> composedProgram;
    session->createCompositeComponentType(reinterpret_cast<slang::IComponentType**>(componentsToLink.data()), (uint32_t)componentsToLink.size(), composedProgram.writeRef(), diagnosticBlob.writeRef());

    return CompileInternal(composedProgram.get(), context);
}

std::expected<Slang::ComPtr<slang::ISession>, Error> SlangCompiler::CreateSession() {
    ComPtr<ISession> session;
    const TargetDesc targetDesc{ .format = SLANG_WGSL, .profile = m_globalSession->findProfile("") };
    std::vector<const char*> searchPaths;
    for (const auto& path : m_paths) searchPaths.push_back(path.c_str());
    const SessionDesc sessionDesc{ .targets = &targetDesc, .targetCount = 1, .searchPaths = searchPaths.data(), .searchPathCount = static_cast<SlangInt>(searchPaths.size()) };
    m_globalSession->createSession(sessionDesc, session.writeRef());
    return session;
}

bool CompilationContext::Failed(SlangResult result, slang::IBlob* diagnosticBlob) {
    if (diagnosticBlob) message += std::string(static_cast<const char*>(diagnosticBlob->getBufferPointer()), diagnosticBlob->getBufferSize());
    return SLANG_FAILED(result);
}
void CompilationContext::AppendError(slang::IBlob* diagnosticBlob) { Failed(SLANG_FAIL, diagnosticBlob); }

} // namespace slangCompiler
'''

with open('shader/SlangCompiler.cpp', 'wb') as f:
    f.write(codecs.BOM_UTF8)
    f.write(content.encode('utf-8'))
