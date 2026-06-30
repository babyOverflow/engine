set(SOURCES "main.cpp" 
    "ExampleLayer.h" "ExampleLayer.cpp"
    "util.h" 
    "Camera3D.h" "Camera3D.cpp" 
    "ModelLoader.h" "ModelLoader.cpp" 
    "ShaderLoader.h" "ShaderLoader.cpp"
)

include (../cmake/ShaderCompiler.cmake)

add_executable (app ${SOURCES})
set(TARGET_NAME "app")

target_compile_features(app PUBLIC cxx_std_23)


target_link_libraries(app PRIVATE core)


#set(SHADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shaders")
#
#add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
#    COMMAND ${CMAKE_COMMAND} -E copy_directory
#    "${SHADER_DIR}"
#    "$<TARGET_FILE_DIR:${TARGET_NAME}>/shaders"
#    
#    COMMENT "Copying Shader files to output directory..."
#)

set(RESOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resources")

add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${RESOURCE_DIR}"
    "$<TARGET_FILE_DIR:${TARGET_NAME}>/resources"
    
    COMMENT "Copying Resource files to output directory..."
)


set(SHADER_TARGET "wgsl")

add_shader_asset(
    TARGET app 
    INPUT "${CMAKE_CURRENT_SOURCE_DIR}/shaders/tri.slang" 
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/assets/tri.shdr"
    INCLUDES "${CORE_INTEROP_HEADER_DIR}"
)

add_shader_asset(
    TARGET app 
    INPUT "${CMAKE_CURRENT_SOURCE_DIR}/shaders/ForwardPass.slang" 
    TEMPLATE "${CMAKE_SOURCE_DIR}/common/entry.slang"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/assets/ForwardPass.shdr"
    INCLUDES "${CORE_INTEROP_HEADER_DIR}" "${CMAKE_SOURCE_DIR}/common" 
)

add_shader_asset(
    TARGET app 
    INPUT "${CMAKE_CURRENT_SOURCE_DIR}/shaders/DeferredGBufferPass.slang" 
    TEMPLATE "${CMAKE_SOURCE_DIR}/common/entry.slang"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/assets/DeferredGBufferPass.shdr"
    INCLUDES "${CORE_INTEROP_HEADER_DIR}" "${CMAKE_SOURCE_DIR}/common" 
)

add_shader_asset(
    TARGET app 
    INPUT "${CMAKE_CURRENT_SOURCE_DIR}/shaders/DeferredLightingPass.slang" 
    TEMPLATE "${CMAKE_SOURCE_DIR}/common/entry.slang"
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/assets/DeferredLightingPass.shdr"
    INCLUDES "${CORE_INTEROP_HEADER_DIR}" "${CMAKE_SOURCE_DIR}/common" 
)

