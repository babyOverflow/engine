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
    "render/resource/GpuResource.h"
    "render/resource/Mesh.h"
    "render/resource/Mesh.cpp"
    "ResourcePool.h"
    "render/graph/RenderGraph.h"
    "render/graph/RenderGraph.cpp"
    "render/backend/LayoutCache.h"
    "render/backend/LayoutCache.cpp"
    "render/backend/PipelineManager.h"
    "render/backend/PipelineManager.cpp"
    "render/resource/ShaderAsset.h"
    "render/resource/ShaderAsset.cpp"
    "render/resource/ShaderManager.h"
    "render/resource/ShaderManager.cpp"
    "render/resource/Material.h"
    "render/resource/Material.cpp"
    "memory/StridedSpan.h"
    "memory/StridedSpan.cpp"
    "util/Load.h"
    "util/Load.cpp"
    "wgx/consts.h"
    "wgx/compare.h"
    "wgx.h"
    "wgx/hash.h"
    "render/util.h"
    "render/util.cpp"
    "wgx/convert.h"
    "import/Importer.h"
    "import/GLTFImporter.h"
    "import/GLTFImporter.cpp"
    "import/ShdrImporter.h"
    "import/ShdrImporter.cpp"
    "render/resource/MaterialManager.h"
    "render/resource/MaterialManager.cpp"
    "render/resource/Texture.h"
    "render/resource/TextureManager.h"
    "render/resource/TextureManager.cpp"
    "render/resource/MeshManager.h"
    "render/resource/MeshManager.cpp"
    "render/resource/Model.h"
    "render/resource/VertexLayoutManager.h"
    "render/resource/VertexLayoutManager.cpp"
    "wgx/types.h"
    "render/SceneRenderer.h"
    "render/SceneRenderer.cpp"
    "Scene.h"
    "render/graph/IRenderPass.cpp"
    "render/pass/DeferredGBufferPass.h"
    "render/pass/DeferredGBufferPass.cpp"
    "render/pass/DeferredLightingPass.h"
    "render/pass/DeferredLightingPass.cpp"
    "render/pass/ForwardRenderPass.h"
    "render/pass/ForwardRenderPass.cpp"
    "render/backend/BindGroupManager.h"
    "render/backend/BindGroupManager.cpp"
    "render/backend/BindGroupFactory.h")

    include(../cmake/ShaderCompiler.cmake)

        set(CORE_INTEROP_HEADER_DIR "${CMAKE_SOURCE_DIR}/common" CACHE INTERNAL
                                    "Include path for shared shader headers")

            file(GLOB_RECURSE _TEMP_HEADERS_CORE_INTEROP_HEADERS CONFIGURE_DEPENDS
                 "${CORE_INTEROP_HEADER_DIR}/*Interop.h" CACHE INTERNAL
                 "List of shared shader headers exported by Core")
                set(CORE_INTEROP_HEADERS ${_TEMP_HEADERS_CORE_INTEROP_HEADERS} CACHE INTERNAL
                    "List of shared shader headers exported by Core")

                    add_library(core STATIC ${SOURCES}) #소스를 여기서 바로 넣는 것이 더 명확합니다.

#[수정 3] 필수 전처리기 정의 추가(NOMINMAX, WIN32_LEAN_AND_MEAN)
#Windows 헤더(winnt.h 등) 와 Dawn / C++ 표준 헤더 충돌 방지를 위해 필수입니다.
#_AMD64_를 수동으로 넣어야 했던 이유는 위쪽 CMP0141 설정 때문일 확률이 높으니
#여기서는 표준적인 Windows 설정만 남깁니다.
    target_compile_definitions(core PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
        target_compile_definitions(core PUBLIC "GLM_FORCE_DEPTH_ZERO_TO_ONE")

#[수정 4] C ++ 표준 설정 통일
#set_property 대신 target_compile_features 사용 권장
            target_compile_features(core PUBLIC cxx_std_23)

#인클루드 디렉터리
                target_include_directories(core PUBLIC "./")

#라이브러리 링크
                    target_link_libraries(core PUBLIC common)
                    target_link_libraries(core PUBLIC glfw)
                    target_link_libraries(core PUBLIC dawn::webgpu_dawn)
                    target_link_libraries(core PRIVATE magic_enum::magic_enum)
                    target_link_libraries(core PUBLIC glm::glm)
                    target_link_libraries(core PUBLIC slang::slang)
                    target_include_directories(core SYSTEM PRIVATE ${TINYGLTF_INCLUDE_DIRS})

#IDE(Visual Studio) 에서 필터 정리
                                source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${SOURCES})

                                    set(PBR_SHDR
                                        "${CMAKE_CURRENT_BINARY_DIR}/asset/StandardPBR.shdr")
                                        set(PBR_HEADER
                                            "${CMAKE_CURRENT_BINARY_DIR}/asset/StandardPBR.h")

                                            add_shader_asset(TARGET core
                                                             INPUT "${CMAKE_CURRENT_SOURCE_DIR}/asset/StandardPBR.slang"
                                                             OUTPUT ${PBR_SHDR}
                                                             INCLUDES "${CORE_INTEROP_HEADER_DIR}")

                                            add_custom_command(
                                                OUTPUT ${PBR_HEADER}
                                                COMMAND python3 "${CMAKE_SOURCE_DIR}/tools/bin2header.py" -i ${PBR_SHDR} -o ${PBR_HEADER} -n kStandardPBR_Data
                                                DEPENDS ${PBR_SHDR}
                                                COMMENT "Embedding StandardPBR into C++ header...")

                                            target_sources(core PRIVATE ${PBR_HEADER})
                                            target_include_directories(core PRIVATE ${CMAKE_CURRENT_BINARY_DIR})