function(add_shader_asset)
    set(options)
    set(oneValueArgs TARGET INPUT ENTRY OUTPUT)
    set(multiValueArgs INCLUDES) # 여러 개의 경로를 받을 수 있음
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # [수정 1] BAKER_TOOL 타겟 수정
    # shaderCompiler는 '라이브러리'입니다. 실행 파일인 'ShaderBaker'를 가리켜야 합니다.
    set(BAKER_TOOL $<TARGET_FILE:shaderCompiler>)

    # [수정 2] INCLUDES 리스트를 CLI 인자 포맷(-I path -I path ...)으로 변환
    set(INCLUDE_FLAGS "")
    foreach(INC_PATH ${ARG_INCLUDES})
        list(APPEND INCLUDE_FLAGS "-I" "${INC_PATH}")
    endforeach()

    add_custom_command(
        OUTPUT ${ARG_OUTPUT}
        COMMAND ${BAKER_TOOL} 
            -i ${ARG_INPUT} 
            -e ${ARG_ENTRY} 
            -o ${ARG_OUTPUT}
            ${INCLUDE_FLAGS}  # [수정 3] 변환된 인자 추가
        MAIN_DEPENDENCY ${ARG_INPUT}
        DEPENDS shaderCompiler # 툴이 먼저 빌드되어야 함
        COMMENT "Baking Shader: ${ARG_INPUT} -> ${ARG_OUTPUT}"
    )

    # 타겟(게임/에디터)이 이 에셋 파일에 의존하도록 설정
    target_sources(${ARG_TARGET} PRIVATE ${ARG_OUTPUT})
endfunction()