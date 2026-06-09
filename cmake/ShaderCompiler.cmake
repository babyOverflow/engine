function(add_shader_asset)
    set(options)
    set(oneValueArgs TARGET INPUT TEMPLATE OUTPUT) # ◀ Replaced ENTRY with TEMPLATE
    set(multiValueArgs INCLUDES) 
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(BAKER_TOOL $<TARGET_FILE:shaderCompiler>)

    set(INCLUDE_FLAGS "")
    foreach(INC_PATH ${ARG_INCLUDES})
        list(APPEND INCLUDE_FLAGS "-I" "${INC_PATH}")
    endforeach()

    set(TEMPLATE_FLAGS "")
    set(DEPENDENCY_LIST shaderCompiler)
    if(ARG_TEMPLATE)
        set(TEMPLATE_FLAGS "-t" "${ARG_TEMPLATE}")
        list(APPEND DEPENDENCY_LIST "${ARG_TEMPLATE}")
    endif()

    add_custom_command(
        OUTPUT ${ARG_OUTPUT}
        COMMAND ${BAKER_TOOL} 
            -i ${ARG_INPUT} 
            ${TEMPLATE_FLAGS}
            -o ${ARG_OUTPUT}
            ${INCLUDE_FLAGS}  
        MAIN_DEPENDENCY ${ARG_INPUT}
        DEPENDS ${DEPENDENCY_LIST} 
        COMMENT "Baking Shader: ${ARG_INPUT} -> ${ARG_OUTPUT}"
    )

    target_sources(${ARG_TARGET} PRIVATE ${ARG_OUTPUT})
endfunction()