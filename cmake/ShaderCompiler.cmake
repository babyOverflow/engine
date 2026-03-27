function(add_shader_asset)
    set(options)
    set(oneValueArgs TARGET INPUT ENTRY OUTPUT)
    set(multiValueArgs INCLUDES) # 여러 개의 경로를 받을 수 있음
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    # [수정 1] BAKER_TOOL 타겟 수정
    set(BAKER_TOOL $<TARGET_FILE:shaderCompiler>)

    # [수정 2] INCLUDES 리스트를 CLI 인자 포맷(-I path -I path ...)으로 변환
    set(INCLUDE_FLAGS "")
    foreach(INC_PATH ${ARG_INCLUDES})
        list(APPEND INCLUDE_FLAGS "-I" "${INC_PATH}")
    endforeach()

    # [추가] ENTRY 옵션이 있는지 확인하고 조건부로 플래그 생성
    set(ENTRY_FLAG "")
    if(ARG_ENTRY)
        list(APPEND ENTRY_FLAG "-e" "${ARG_ENTRY}")
        # 또는 set(ENTRY_FLAG "-e" "${ARG_ENTRY}") 도 동일하게 작동합니다.
    endif()

    add_custom_command(
        OUTPUT ${ARG_OUTPUT}
        COMMAND ${BAKER_TOOL} 
            -i ${ARG_INPUT} 
            ${ENTRY_FLAG}       # [수정됨] 값이 없으면 빈 공간으로 무시됨
            -o ${ARG_OUTPUT}
            ${INCLUDE_FLAGS}  
        MAIN_DEPENDENCY ${ARG_INPUT}
        DEPENDS shaderCompiler # 툴이 먼저 빌드되어야 함
        COMMENT "Baking Shader: ${ARG_INPUT} -> ${ARG_OUTPUT}"
    )

    # 타겟(게임/에디터)이 이 에셋 파일에 의존하도록 설정
    target_sources(${ARG_TARGET} PRIVATE ${ARG_OUTPUT})
endfunction()