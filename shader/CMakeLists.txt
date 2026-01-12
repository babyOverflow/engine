

add_library(shader    "SlangCompiler.h" "SlangCompiler.cpp")

target_compile_features(shader PUBLIC cxx_std_23)
target_link_libraries(shader PUBLIC slang::slang core)
target_include_directories(shader PUBLIC "./")

add_executable(shaderCompiler "main.cpp")
target_link_libraries(shaderCompiler PRIVATE  shader)

add_custom_command(TARGET shaderCompiler  POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E  copy_if_different
	"$<TARGET_FILE_DIR:slang::slang>/slang.dll"
	"$<TARGET_FILE_DIR:shaderCompiler>/"
)


enable_testing()

add_executable(shaderTest "test/test.cpp" "test/ShaderInterop.h" "test/test.h")
target_link_libraries(shaderTest PRIVATE  shader)
target_link_libraries(shaderTest PRIVATE GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main)

include(GoogleTest)
gtest_discover_tests(shaderTest)
