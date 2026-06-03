#include <ShaderAssetFormat.h>
#include <array>

#include <gtest/gtest.h>
#include <source_location>

#include "SlangCompiler.h"

using namespace slangCompiler;

using sa = core::ShaderAssetFormat;

class PassTest : public testing::Test {
  protected:
    static Slang::ComPtr<slang::IGlobalSession> globalSession;
    static Slang::ComPtr<slang::ISession> session;

    static void SetUpTestSuite() {
        using namespace slang;
        using Slang::ComPtr;

        // 1. 글로벌 세션 초기화
        SlangGlobalSessionDesc desc{};
        SlangResult result = createGlobalSession(&desc, globalSession.writeRef());
        ASSERT_FALSE(SLANG_FAILED(result)) << "Failed to create global session";

        // 2. 세션 디스크립터 설정 (WGSL 타겟 명시)
        const TargetDesc targetDesc{
            .format = SLANG_WGSL,
            .profile = globalSession->findProfile("glsl_450"),  // 빈 문자열 대신 구체적 프로필 지정
            .flags = SLANG_TARGET_FLAG_GENERATE_WHOLE_PROGRAM |
                     SLANG_TARGET_FLAG_PARAMETER_BLOCKS_USE_REGISTER_SPACES,
        };

        std::vector<CompilerOptionEntry> options;
        options.emplace_back(
            CompilerOptionEntry{.name = CompilerOptionName::PreserveParameters,
                                .value = {.kind = CompilerOptionValueKind::Int, .intValue0 = 1}});
        options.emplace_back(
            CompilerOptionEntry{.name = CompilerOptionName::EmitReflectionJSON,
                                .value = {.kind = CompilerOptionValueKind::Int, .intValue0 = 1}});
        options.emplace_back(
            CompilerOptionEntry{.name = CompilerOptionName::MatrixLayoutColumn,
                                .value = {.kind = CompilerOptionValueKind::Int, .intValue0 = 1}});
        options.emplace_back(
            CompilerOptionEntry{.name = CompilerOptionName::EnableEffectAnnotations,
                                .value = {.kind = CompilerOptionValueKind::Int, .intValue0 = 1}});
        options.emplace_back(
            CompilerOptionEntry{.name = CompilerOptionName::VulkanEmitReflection,
                                .value = {.kind = CompilerOptionValueKind::Int, .intValue0 = 1}});
        // -------------------------------------------------------------------------
        // [수정 포인트] 문자열 객체의 수명을 세션이 생성될 때까지 완벽하게 보존합니다.
        // -------------------------------------------------------------------------
        const auto loc = std::source_location::current();
        const std::filesystem::path fullPath(loc.file_name());
        const std::filesystem::path dirPath = fullPath.parent_path();

        // 임시 rvalue가 아니라 독립 변수로 문자열 객체를 선언하여 주소가 깨지는 것을 방어합니다.
        std::string searchPathStr = dirPath.string();
        const char* searchPathRaw = searchPathStr.c_str();

        const SessionDesc sessionDesc{
            .targets = &targetDesc,
            .targetCount = 1,
            .searchPaths = &searchPathRaw,
            .searchPathCount = 1,
            .compilerOptionEntries = options.data(),
            .compilerOptionEntryCount = static_cast<uint32_t>(options.size()),
        };

        result = globalSession->createSession(sessionDesc, session.writeRef());
        ASSERT_FALSE(SLANG_FAILED(result)) << "Failed to create session";
    }
};

// 정적 멤버 변수 정의
Slang::ComPtr<slang::IGlobalSession> PassTest::globalSession = nullptr;
Slang::ComPtr<slang::ISession> PassTest::session = nullptr;

// 테스트에 주입할 실전 Slang 셰이더 소스 (앞선 세션의 아키텍처 반영)

// 2. 유저 패스 구현 소스 (GBufferPass.slang)
const char* kPassesShader = R"(
#include "core_interface.slang" // 에디터 인텔리센스용 인클루드 유지

struct DiffuseMaterial : IMaterial {
    typealias DataType = struct {
        Texture2D diffuseMap;
        SamplerState linearSampler;
    };
    static SurfaceAttributes evaluateSurfaceAttributes(DataType data,
                                                       float3 worldPosition,
                                                       float3 worldNormal,
                                                       float2 uv) {
        SurfaceAttributes attributes;
        attributes.baseColor = data.diffuseMap.Sample(data.linearSampler, uv).rgb;
        return attributes;
    }
};

struct GBufferPass<Material : IMaterial> : IRenderPass {
    typealias M = Material;
    typealias VertexInputType = struct {
        float3 position : POSITION;
        float3 normal : NORMAL;
        float2 uv : TEXCOORD;
    };
    typealias VertexOutputType = struct {
        float4 position : SV_Position;
        float3 worldPos : TEXCOORD0;
        float3 normal : TEXCOORD1;
        float2 uv : TEXCOORD2;
    };
    typealias FragmentInputType = struct {
        float3 worldPos : TEXCOORD0;
        float3 normal : TEXCOORD1;
        float2 uv : TEXCOORD2;
    };
    typealias FragmentOutputType = struct GBufferColors {
        float4 albedo : SV_Target0;
        float4 normal : SV_Target1;
        float4 linearDepth : SV_Target2;
    };
    typealias GlobalResourcesType = GlobalResources;
    typealias PassResourceType = EmptyData;

    static VertexOutputType vertex(GlobalResourcesType globalResources,
                                   M.DataType material,
                                   PassResourceType passResources,
                                   VertexInputType input) {
        VertexOutputType output;
        float4 worldPos = float4(input.position, 1.0);
        output.position = mul(worldPos, globalResources.viewProjMatrix);
        output.worldPos = input.position;
        output.normal = input.normal;
        output.uv = input.uv;
        return output;
    }

    static FragmentOutputType fragment(GlobalResourcesType globalResources,
                                       M.DataType material,
                                       PassResourceType passResources,
                                       FragmentInputType input) {
        GBufferColors outTargets;
        SurfaceAttributes surfaceAttrs =
            M.evaluateSurfaceAttributes(material, input.worldPos, input.normal, input.uv);
        outTargets.albedo = float4(surfaceAttrs.baseColor, 1.0);
        outTargets.normal = float4(input.normal * 0.5 + 0.5, 1.0);
        outTargets.linearDepth = float4(input.worldPos.z, 0.0, 0.0, 1.0);
        return outTargets;
    }
}
)";

// 3. 하드유저 친화적인 안전장치 내장 고정 엔트리 청사진 (core.slang)
const char* kEntryShader = R"(
#include <ShaderInterop.h>
#include "core_interface.slang"

#ifndef ENGINE_STATIC_DISPATCH
type_param P : IRenderPass;
#endif

[vk::binding(0, 0)]
ParameterBlock<P.GlobalResourcesType> globalResources;

[vk::binding(0, 1)]
ParameterBlock<P.M.DataType> material;

[vk::binding(0, 2)]
ParameterBlock<P.PassResourceType> currentPass;

[shader("vertex")]
P.VertexOutputType vertexMain(P.VertexInputType input) {
    return P.vertex(globalResources, material, currentPass, input);
}

[shader("fragment")]
P.FragmentOutputType fragmentMain(P.FragmentInputType input) {
    return P.fragment(globalResources, material, currentPass, input);
}

)";

TEST_F(PassTest, Generic) {
    using namespace slang;
    using Slang::ComPtr;

    ComPtr<IBlob> diagnosticBlob;
    SlangResult result;

    auto passModule = Slang::ComPtr<IModule>(session->loadModuleFromSourceString(
        "tempModule", "tempPath", kPassesShader, diagnosticBlob.writeRef()));
    ASSERT_NE(passModule.get(), nullptr);

    slang::ShaderReflection* layout = passModule->getLayout();

    slang::TypeReflection* genericPassType = layout->findTypeByName("GBufferPass");
    slang::TypeReflection* renderPassInterface = layout->findTypeByName("IRenderPass");
    ASSERT_EQ(genericPassType->getKind(), slang::TypeReflection::Kind::Struct);

    // 2. [확보 플로우 1단계]: 제네릭 구조체의 'Generic 템플릿 컨텍스트 틀' 추출
    slang::GenericReflection* genericContainer = genericPassType->getGenericContainer();
    ASSERT_NE(genericContainer, nullptr);

    // 3. 특수화 인자로 주입할 구체적 머티리얼 타입 확보
    slang::TypeReflection* concreteMaterialType = layout->findTypeByName("DiffuseMaterial");
    ASSERT_NE(concreteMaterialType, nullptr);

    // 4. [확보 플로우 2단계]: slang.h 명세에 맞춰 제네릭 인자(Arguments) 구조체 배열 조립
    slang::GenericArgType argType = slang::GenericArgType::SLANG_GENERIC_ARG_TYPE;  // 0: Type 지정

    slang::GenericArgReflection argVal;
    argVal.typeVal = concreteMaterialType;  // 공용체(Union) 멤버에 구체적 머티리얼 포인터 주입

    // 5. [확보 플로우 3단계]: ShaderReflection 인터페이스를 트리거하여 인메모리 특수화 바인딩
    // 컨텍스트 연산
    Slang::ComPtr<slang::IBlob> specializeDiag;
    slang::GenericReflection* specializedGenericContext = layout->specializeGeneric(
        genericContainer,                                // 원본 제네릭 컨텍스트 틀
        1,                                               // 주입할 인자 개수
        (SlangReflectionGenericArgType const*)&argType,  // 인자 타입 배열 포인터
        &argVal,                                         // 인자 값 배열 포인터
        specializeDiag.writeRef());
    ASSERT_NE(specializedGenericContext, nullptr)
        << (specializeDiag ? (const char*)specializeDiag->getBufferPointer()
                           : "Failed to specialize generic");

    // 6. 유저가 지목한 applySpecializations 메서드를 통해 드디어 Kind::Struct 구체 타입 변환 성공
    slang::TypeReflection* specializedStructType =
        genericPassType->applySpecializations(specializedGenericContext);
    ASSERT_EQ(specializedStructType->getKind(), slang::TypeReflection::Kind::Struct);

    // 7. 최종 런타임 인터페이스 정합성 검사 수행 (가상 소스코드 주익 우회책 전면 폐기 가능)
    Slang::ComPtr<slang::ITypeConformance> conformanceComponent;
    SlangResult conformanceResult = session->createTypeConformanceComponentType(
        specializedStructType, renderPassInterface, conformanceComponent.writeRef(), 0, nullptr);

    ASSERT_TRUE(SLANG_SUCCEEDED(conformanceResult));
}

TEST_F(PassTest, RenderPassStructureShouldSucceedConformanceVerification) {
    using namespace slang;
    using Slang::ComPtr;

    ComPtr<IBlob> diagnosticBlob;
    SlangResult result;

    auto passModule = Slang::ComPtr<IModule>(session->loadModuleFromSourceString(
        "tempModule", "tempPath", kPassesShader, diagnosticBlob.writeRef()));
    ASSERT_NE(passModule.get(), nullptr);

    slang::ShaderReflection* layout = passModule->getLayout();

    // 1. Fetch unspecialized types
    slang::TypeReflection* genericPassType = layout->findTypeByName("GBufferPass");
    slang::TypeReflection* renderPassInterface = layout->findTypeByName("IRenderPass");
    slang::TypeReflection* concreteMaterialType =
        layout->findTypeByName("EmptyMaterial");  // ◀ Target material specified

    ASSERT_NE(genericPassType, nullptr) << "Failed to find GBufferPass type";
    ASSERT_NE(renderPassInterface, nullptr) << "Failed to find IRenderPass interface";
    ASSERT_NE(concreteMaterialType, nullptr) << "Failed to find EmptyMaterial type";

    // 2. Extract the generic container blueprint from the pass factory
    slang::GenericReflection* genericContainer = genericPassType->getGenericContainer();
    ASSERT_NE(genericContainer, nullptr) << "Failed to extract generic container from GBufferPass";

    // 3. Assemble the specialization argument layout vectors
    slang::GenericArgType argType = slang::GenericArgType::SLANG_GENERIC_ARG_TYPE;
    slang::GenericArgReflection argVal;
    argVal.typeVal = concreteMaterialType;

    // 4. Generate the in-memory specialized binding context
    ComPtr<IBlob> specializeDiag;
    slang::GenericReflection* specializedGenericContext = layout->specializeGeneric(
        genericContainer, 1, (SlangReflectionGenericArgType const*)&argType, &argVal,
        specializeDiag.writeRef());

    ASSERT_NE(specializedGenericContext, nullptr)
        << "In-memory specialization context mapping failed:\n"
        << (specializeDiag ? (const char*)specializeDiag->getBufferPointer()
                           : "No diagnostic info");

    // 5. Instanciate the concrete type snapshot from the generic master definition
    slang::TypeReflection* specializedStructType =
        genericPassType->applySpecializations(specializedGenericContext);
    ASSERT_NE(specializedStructType, nullptr)
        << "Failed to apply specialization context to type layer";
    ASSERT_EQ(specializedStructType->getKind(), slang::TypeReflection::Kind::Struct);

    // 6. Execute runtime interface witness table conformance verification
    Slang::ComPtr<slang::ITypeConformance> conformanceComponent;
    Slang::ComPtr<slang::IBlob> conformanceDiagnosticBlob;

    SlangResult conformanceResult = session->createTypeConformanceComponentType(
        specializedStructType, renderPassInterface, conformanceComponent.writeRef(), 0,
        conformanceDiagnosticBlob.writeRef());

    ASSERT_TRUE(SLANG_SUCCEEDED(conformanceResult))
        << "Type consistency check failed! The specialized concrete struct does not implement "
           "IRenderPass.\n"
        << (conformanceDiagnosticBlob ? (const char*)conformanceDiagnosticBlob->getBufferPointer()
                                      : "No log info");
}

TEST_F(PassTest, NonRenderPassStructureShouldFailConformanceVerification) {
    using namespace slang;
    using Slang::ComPtr;

    ComPtr<IBlob> diagnosticBlob;
    SlangResult result;

    auto passModule = Slang::ComPtr<IModule>(session->loadModuleFromSourceString(
        "tempModule", "tempPath", kPassesShader, diagnosticBlob.writeRef()));
    ASSERT_NE(passModule.get(), nullptr);

    slang::ShaderReflection* layout = passModule->getLayout();

    slang::TypeReflection* nonPassStructType = layout->findTypeByName("GlobalResources");
    ASSERT_NE(nonPassStructType, nullptr)
        << "GlobalResources struct should exist in the source for this negative test.";

    slang::TypeReflection* renderPassInterface = layout->findTypeByName("IRenderPass");
    ASSERT_NE(renderPassInterface, nullptr);

    ComPtr<ITypeConformance> conformanceComponent;
    ComPtr<ISlangBlob> conformanceDiagnosticBlob;

    result = session->createTypeConformanceComponentType(nonPassStructType, renderPassInterface,
                                                         conformanceComponent.writeRef(), 0,
                                                         conformanceDiagnosticBlob.writeRef());

    EXPECT_TRUE(SLANG_FAILED(result)) << "정합성 검증 유출 발생: IRenderPass를 상속받지 않은 일반 "
                                         "구조체가 유효한 패스로 통과되었습니다!";

    // 추가 검증: 컴파일러 에러 다이아그노스틱 블롭이 null이 아니며 실패 사유(예: 상속 부족 등)를
    // 들고 있는지 확인
    if (conformanceDiagnosticBlob) {
        std::cout << "정상적으로 실패 로그가 생성되었습니다: \n"
                  << (const char*)conformanceDiagnosticBlob->getBufferPointer();
    }
}

TEST_F(PassTest, CompleteInterfaceSeparatedCompilationVerificationd) {
    using namespace slang;
    using Slang::ComPtr;

    const auto loc = std::source_location::current();
    std::filesystem::path testDir = std::filesystem::path(loc.file_name()).parent_path();
    ASSERT_TRUE(std::filesystem::exists(testDir / "core_interface.slang"))
        << "테스트 실행 전 'core_interface.slang' 파일이 디스크 경로에 물리적으로 존재해야 합니다!";

    ComPtr<IBlob> diagnosticBlob;
    ComPtr<IBlob> wgslCodeBlob;
    SlangResult result;

    // =========================================================================
    // Phase 2: 컴파일러 네이티브 AST 검증을 통한 Pass 및 Material 이름 자동 식별
    // =========================================================================
    auto passModule = ComPtr<IModule>(session->loadModuleFromSourceString(
        "GBufferPassModule", "GBufferPassModule.slang", kPassesShader, diagnosticBlob.writeRef()));
    ASSERT_NE(passModule.get(), nullptr) << (const char*)diagnosticBlob->getBufferPointer();

    slang::DeclReflection* moduleDecl = passModule->getModuleReflection();
    slang::ShaderReflection* layout = passModule->getLayout();
    slang::TypeReflection* renderPassInterface = layout->findTypeByName("IRenderPass");
    slang::TypeReflection* materialInterface = layout->findTypeByName("IMaterial");

    std::string detectedPassName = "";
    std::string detectedMaterialName = "";
    auto children = moduleDecl->getChildren();

    // 1단계: IMaterial을 구현하는 구체적(Concrete) 머티리얼 타입 구조체 먼저 식별
    slang::TypeReflection* concreteMaterialType = nullptr;
    for (slang::DeclReflection* childDecl : children) {
        if (childDecl && childDecl->getKind() == slang::DeclReflection::Kind::Struct &&
            childDecl->getName() == std::string("DiffuseMaterial")) {
            slang::TypeReflection* currentType = layout->findTypeByName(childDecl->getName());
            if (!currentType) {
                continue;
            }

            ComPtr<ITypeConformance> conformanceComponent;
            ComPtr<ISlangBlob> dummyDiagnostics;

            SlangResult matResult = session->createTypeConformanceComponentType(
                currentType, materialInterface, conformanceComponent.writeRef(), 0,
                dummyDiagnostics.writeRef());

            if (SLANG_SUCCEEDED(matResult)) {
                detectedMaterialName = childDecl->getName();
                concreteMaterialType = currentType;
                break;
            }
        }
    }

    ASSERT_FALSE(detectedMaterialName.empty()) << "IMaterial 구현체를 식별하지 못했습니다.";
    EXPECT_EQ(detectedMaterialName, "DiffuseMaterial");
    // 2단계: 식별된 머티리얼 타입을 기반으로 제네릭 Pass를 인메모리 특수화하여 IRenderPass 정합성
    // 검증
    for (slang::DeclReflection* childDecl : children) {
        if (childDecl && (childDecl->getKind() == slang::DeclReflection::Kind::Generic ||
                          childDecl->getKind() == slang::DeclReflection::Kind::Struct)) {
            slang::TypeReflection* currentType = layout->findTypeByName(childDecl->getName());
            if (!currentType) {
                continue;
            }

            slang::GenericReflection* genericContainer = currentType->getGenericContainer();
            slang::TypeReflection* typeToCheck = currentType;

            // 제네릭 컨텍스트 팩토리가 존재할 경우 인메모리 조립 단행
            if (genericContainer) {
                slang::GenericArgType argType = slang::GenericArgType::SLANG_GENERIC_ARG_TYPE;
                slang::GenericArgReflection argVal;
                argVal.typeVal = concreteMaterialType;

                ComPtr<IBlob> specDiag;
                slang::GenericReflection* specContext = layout->specializeGeneric(
                    genericContainer, 1, (SlangReflectionGenericArgType const*)&argType, &argVal,
                    specDiag.writeRef());

                if (specContext) {
                    typeToCheck = currentType->applySpecializations(specContext);
                }
            }

            if (!typeToCheck) {
                continue;
            }

            ComPtr<ITypeConformance> conformanceComponent;
            ComPtr<ISlangBlob> dummyDiagnostics;

            SlangResult passResult = session->createTypeConformanceComponentType(
                typeToCheck, renderPassInterface, conformanceComponent.writeRef(), 0,
                dummyDiagnostics.writeRef());

            if (SLANG_SUCCEEDED(passResult)) {
                detectedPassName = childDecl->getName();
                break;
            }
        }
    }

    ASSERT_FALSE(detectedPassName.empty()) << "IRenderPass 구현체를 식별하지 못했습니다.";
    EXPECT_EQ(detectedPassName, "GBufferPass");

    // =========================================================================
    // Phase 3: 하드유저 방어선 무력화 매크로 및 typealias P 동적 꼬리 주입 (Append)
    // =========================================================================
    std::string macroControlCode =
        "\n\n// Injected by Framework\n"
        "#define ENGINE_STATIC_DISPATCH\n"
        "typealias P = " +
        detectedPassName + "<" + detectedMaterialName + ">;\n";

    std::string finalCombinedSource =
        std::string(kPassesShader) + macroControlCode + std::string(kEntryShader);

    auto combinedModule = ComPtr<IModule>(session->loadModuleFromSourceString(
        "finalCombinedModule", "core.slang", finalCombinedSource.c_str(),
        diagnosticBlob.writeRef()));

    ASSERT_NE(combinedModule.get(), nullptr) << "최종 결합 컴파일 실패 로그: \n"
                                             << (const char*)diagnosticBlob->getBufferPointer();

    // =========================================================================
    // Phase 4: 다중 진입점 링킹 및 단일 오차 없는 마스터 WGSL 배출
    // =========================================================================
    ComPtr<IEntryPoint> vertexEntry;
    result = combinedModule->findEntryPointByName("vertexMain", vertexEntry.writeRef());
    ASSERT_TRUE(SLANG_SUCCEEDED(result));

    ComPtr<IEntryPoint> fragmentEntry;
    result = combinedModule->findEntryPointByName("fragmentMain", fragmentEntry.writeRef());
    ASSERT_TRUE(SLANG_SUCCEEDED(result));

    std::vector<IComponentType*> componentList = {combinedModule.get(), vertexEntry.get(),
                                                  fragmentEntry.get()};

    ComPtr<IComponentType> compositeProgram;
    result = session->createCompositeComponentType(componentList.data(), componentList.size(),
                                                   compositeProgram.writeRef(),
                                                   diagnosticBlob.writeRef());
    ASSERT_TRUE(SLANG_SUCCEEDED(result));

    ComPtr<IComponentType> linkedProgram;
    result = compositeProgram->link(linkedProgram.writeRef(), diagnosticBlob.writeRef());
    ASSERT_TRUE(SLANG_SUCCEEDED(result));

    result = linkedProgram->getTargetCode(0, wgslCodeBlob.writeRef(), diagnosticBlob.writeRef());
    ASSERT_TRUE(SLANG_SUCCEEDED(result)) << (const char*)diagnosticBlob->getBufferPointer();

    ASSERT_NE(wgslCodeBlob.get(), nullptr);

    std::string wgslCode((const char*)wgslCodeBlob->getBufferPointer(),
                         wgslCodeBlob->getBufferSize());
    std::cout << "Generated WGSL Code:\n" << wgslCode;

    ComPtr<IBlob> reflectionBlob;

    linkedProgram->getLayout()->toJson(reflectionBlob.writeRef());
    ASSERT_TRUE(SLANG_SUCCEEDED(result)) << (const char*)diagnosticBlob->getBufferPointer();
    std::string reflectionJson((const char*)reflectionBlob->getBufferPointer(),
                               reflectionBlob->getBufferSize());
    std::cout << "Generated Reflection JSON:\n" << reflectionJson;
}

// int main(int argc, char** argv) {
//     std::cout << ">>> Slang Shader Tests" << "\n";
//     ::testing::InitGoogleTest(&argc, argv);

//     int result = RUN_ALL_TESTS();

//     std::cout << ">>> result: " << result << "\n";
//     return result;
// }
