set(SOURCES
    "render/render.cpp"
    "render/render.h"
    "Application.cpp"
    "Application.h" 
    "Layer.h"
    "Window.h" 
    "Window.cpp"
    "Event.h"
    "Event.cpp"
    "event/InputEvent.h"
    "event/WindowEvent.h"
    "AssetManager.h"
    "AssetManager.cpp" 
    "render/util.h"
    "render/GpuResource.h"
    "render/Mesh.h" 
    "render/Mesh.cpp" 
    "ResourcePool.h" 
    "render/RenderGraph.h"
    "render/RenderGraph.cpp" 
    "render/LayoutCache.h"
    "render/LayoutCache.cpp"
    "render/ShaderInterop.h" 
    "render/PipelineManager.h"
    "render/PipelineManager.cpp"
    "memory/StridedSpan.h"
    "memory/StridedSpan.cpp" 
    "util/Load.h" 
    "util/Load.cpp" 
    "wgx/consts.h"
    "wgx.h"
    "wgx/hash.h"
    "ShaderAssetFormat.h"
    "ShaderAssetFormat.cpp" "Common.h")


set(CORE_INTEROP_HEADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/render"
    CACHE INTERNAL "Include path for shared shader headers")

file(GLOB_RECURSE _TEMP_HEADERS_CORE_INTEROP_HEADERS 
     CONFIGURE_DEPENDS 
     "${CORE_INTEROP_HEADER_DIR}/*Interop.h"
    CACHE INTERNAL "List of shared shader headers exported by Core")
set(CORE_INTEROP_HEADERS ${_TEMP_HEADERS_CORE_INTEROP_HEADERS} 
    CACHE INTERNAL "List of shared shader headers exported by Core")


add_library(core STATIC ${SOURCES}) # 소스를 여기서 바로 넣는 것이 더 명확합니다.

# [수정 3] 필수 전처리기 정의 추가 (NOMINMAX, WIN32_LEAN_AND_MEAN)
# Windows 헤더(winnt.h 등)와 Dawn/C++ 표준 헤더 충돌 방지를 위해 필수입니다.
# _AMD64_를 수동으로 넣어야 했던 이유는 위쪽 CMP0141 설정 때문일 확률이 높으니
# 여기서는 표준적인 Windows 설정만 남깁니다.
target_compile_definitions(core PRIVATE 
    NOMINMAX 
    WIN32_LEAN_AND_MEAN
)
target_compile_definitions(core PUBLIC "GLM_FORCE_DEPTH_ZERO_TO_ONE")


# [수정 4] C++ 표준 설정 통일
# set_property 대신 target_compile_features 사용 권장
target_compile_features(core PUBLIC cxx_std_23)

# 인클루드 디렉터리
target_include_directories(core PUBLIC "./")

# 라이브러리 링크
target_link_libraries(core PUBLIC glfw)
target_link_libraries(core PUBLIC dawn::webgpu_dawn)
target_link_libraries(core PRIVATE magic_enum::magic_enum)
target_link_libraries(core PUBLIC glm::glm)
target_link_libraries(core PUBLIC slang::slang)
target_include_directories(core PRIVATE ${TINYGLTF_INCLUDE_DIRS})

# IDE(Visual Studio)에서 필터 정리
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SOURCES})
