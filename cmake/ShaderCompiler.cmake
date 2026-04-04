function(add_shader_asset)
    set(options)
    set(oneValueArgs TARGET INPUT ENTRY OUTPUT)
    set(multiValueArgs INCLUDES) 
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(BAKER_TOOL $<TARGET_FILE:shaderCompiler>)

    set(INCLUDE_FLAGS "")
    foreach(INC_PATH ${ARG_INCLUDES})
        list(APPEND INCLUDE_FLAGS "-I" "${INC_PATH}")
    endforeach()

    set(ENTRY_FLAG "")
    if(ARG_ENTRY)
        list(APPEND ENTRY_FLAG "-e" "${ARG_ENTRY}")
    endif()

    add_custom_command(
        OUTPUT ${ARG_OUTPUT}
        COMMAND ${BAKER_TOOL} 
            -i ${ARG_INPUT} 
            ${ENTRY_FLAG}
            -o ${ARG_OUTPUT}
            ${INCLUDE_FLAGS}  
        MAIN_DEPENDENCY ${ARG_INPUT}
        DEPENDS shaderCompiler
        COMMENT "Baking Shader: ${ARG_INPUT} -> ${ARG_OUTPUT}"
    )

    target_sources(${ARG_TARGET} PRIVATE ${ARG_OUTPUT})
endfunction()