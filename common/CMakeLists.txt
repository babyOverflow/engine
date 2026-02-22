
add_library(common STATIC 
	"ShaderAssetFormat.h" "ShaderAssetFormat.cpp"
	"Common.h"
 "TextureAssetFormat.h" 
	"MaterialAssetFormat.h"
	"MeshAssetFormat.h"
	"ModelAssetFormat.h"
)
target_include_directories(common INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(common PUBLIC magic_enum::magic_enum)

# (옵션) C++ 버전 설정
target_compile_features(common PUBLIC cxx_std_23)