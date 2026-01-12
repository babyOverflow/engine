set(SOURCES "main.cpp" "ExampleLayer.h" "ExampleLayer.cpp"  "util.h" "Camera3D.h" "Camera3D.cpp")

include (../cmake/ShaderCompiler.cmake)

add_executable (app ${SOURCES})
set(TARGET_NAME "app")

target_compile_features(app PUBLIC cxx_std_23)


# Core 라이브러리 링크
target_link_libraries(app PRIVATE core)


#set(SHADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shaders")
#
#add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
#    # CMake의 내장 복사 명령 사용 (OS 무관하게 동작, xcopy/cp 불필요)
#    COMMAND ${CMAKE_COMMAND} -E copy_directory
#    "${SHADER_DIR}"
#    "$<TARGET_FILE_DIR:${TARGET_NAME}>/shaders"
#    
#    # 주석 (Visual Studio 빌드 로그에 찍힘)
#    COMMENT "Copying Shader files to output directory..."
#)

set(RESOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resources")

add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
    # CMake의 내장 복사 명령 사용 (OS 무관하게 동작, xcopy/cp 불필요)
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${RESOURCE_DIR}"
    "$<TARGET_FILE_DIR:${TARGET_NAME}>/resources"
    
    # 주석 (Visual Studio 빌드 로그에 찍힘)
    COMMENT "Copying Resource files to output directory..."
)


set(SHADER_TARGET "wgsl")

function(add_slang_shader TARGET_NAME SHADER_FILE)
    set(SHADER_SOURCE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/shaders")
    set(SHADER_OUTPUT_ROOT "${CMAKE_CURRENT_BINARY_DIR}/shaders")
    
    # 절대 경로 및 상대 경로 계산
    get_filename_component(ABS_FILE "${SHADER_FILE}" ABSOLUTE)
    file(RELATIVE_PATH REL_PATH "${SHADER_SOURCE_ROOT}" "${ABS_FILE}")
    
    set(OUT_FILE "${SHADER_OUTPUT_ROOT}/${REL_PATH}.${SHADER_TARGET}")
    get_filename_component(OUT_DIR "${OUT_FILE}" DIRECTORY)
    
    add_custom_command(
        OUTPUT "${OUT_FILE}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${OUT_DIR}"
        COMMAND slangc "${ABS_FILE}" 
                -target "${SHADER_TARGET}"
                -o "${OUT_FILE}"
                -I "${CORE_INTEROP_HEADER_DIR}"
        DEPENDS "${ABS_FILE}" "${CORE_INTEROP_HEADERS}"
        COMMENT "Building Shader: ${REL_PATH}"
    )
    
    # 타겟에 소스로 추가 (CMake가 알아서 커스텀 커맨드 의존성을 추적함)
    target_sources(${TARGET_NAME} PRIVATE "${OUT_FILE}")
    
    # 필요하다면 매크로 정의 추가 (파일별로 구별할 식별자가 필요함)
    # 예: SHADER_PATH_TRI_SLANG 처럼 이름을 대문자로 바꿔서 정의
    string(MAKE_C_IDENTIFIER "${REL_PATH}" PATH_ID)
    string(TOUPPER "${PATH_ID}" PATH_ID)
    target_compile_definitions(${TARGET_NAME} PRIVATE "SHADER_PATH_${PATH_ID}=\"${OUT_FILE}\"")
endfunction()

# 사용법
#add_slang_shader(app "shaders/tri.slang")

# new shader asset format
add_shader_asset(
    TARGET app 
    INPUT "${CMAKE_CURRENT_SOURCE_DIR}/shaders/tri.slang" 
    ENTRY "vertexMain"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/assets/tri_VS.shdr"
    INCLUDES "${CORE_INTEROP_HEADER_DIR}"
)

add_shader_asset(
    TARGET app 
    INPUT "${CMAKE_CURRENT_SOURCE_DIR}/shaders/tri.slang" 
    ENTRY "fragmentMain"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/assets/tri_FS.shdr"
    INCLUDES "${CORE_INTEROP_HEADER_DIR}"
)