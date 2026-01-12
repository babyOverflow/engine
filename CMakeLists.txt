# CMakeList.txt : 최상위 CMake 프로젝트 파일, 전역 구성을 수행하고
# 여기에 하위 프로젝트를 포함합니다.
#
cmake_minimum_required (VERSION 3.12)

set(CMAKE_CXX_CLANG_TIDY 
    "clang-tidy"
    # [핵심 1] Clang에게 "나는 cl.exe처럼 행동할 거야"라고 알려줌 (MSVC 모드)
    "--extra-arg=--driver-mode=cl"
    
    # [핵심 2] MSVC가 내부적으로 정의하는 아키텍처 매크로 강제 주입
    # 이게 없으면 winnt.h가 64비트 함수 매핑을 건너뜁니다.
    "--extra-arg=-D_M_AMD64"
    "--extra-arg=-D_M_X64"
    "--extra-arg=-fms-extensions"
    "--extra-arg=-fms-compatibility"
    "--extra-arg=-Wno-ignored-attributes" 
    "--config-file=${CMAKE_SOURCE_DIR}/.clang-tidy"
)

# 지원되는 경우 MSVC 컴파일러에 대해 핫 다시 로드 사용하도록 설정합니다.
#if (POLICY CMP0141)
#  cmake_policy(SET CMP0141 NEW)
#  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
#endif()
project ("render")
add_compile_definitions(
    _AMD64_                  # 강제로 x64 아키텍처 명시
    NOMINMAX                 # min/max 매크로 충돌 방지
    WIN32_LEAN_AND_MEAN      # 불필요한 윈도우 API 제외
    _CRT_SECURE_NO_WARNINGS  # strcpy 등 경고 무시
    GLFW_INCLUDE_NONE
)

find_package(glfw3 CONFIG REQUIRED)
find_package(dawn CONFIG REQUIRED)
find_package(magic_enum CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(slang CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)

find_path(TINYGLTF_INCLUDE_DIRS "tiny_gltf.h")

# 하위 프로젝트를 포함합니다.
add_subdirectory("core")
add_subdirectory("shader")
add_subdirectory("app")
