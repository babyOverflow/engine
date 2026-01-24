#pragma once
#include <dawn/webgpu_cpp.h>

namespace wgx {
inline size_t GetUnitSize(wgpu::TextureFormat format) {
    constexpr size_t kUnKnown = -1;
    constexpr size_t kOneBytes = sizeof(char);
    constexpr size_t kTwoBytes = 2 * kOneBytes;
    constexpr size_t kFourBytes = 4 * kOneBytes;

    switch (format) {
        case wgpu::TextureFormat::Undefined:
            return kUnKnown;
        case wgpu::TextureFormat::R8Unorm:
        case wgpu::TextureFormat::R8Snorm:
        case wgpu::TextureFormat::R8Uint:
        case wgpu::TextureFormat::R8Sint:
            return kOneBytes;
        case wgpu::TextureFormat::R16Unorm:
        case wgpu::TextureFormat::R16Snorm:
        case wgpu::TextureFormat::R16Uint:
        case wgpu::TextureFormat::R16Sint:
        case wgpu::TextureFormat::R16Float:
        case wgpu::TextureFormat::RG8Unorm:
        case wgpu::TextureFormat::RG8Snorm:
        case wgpu::TextureFormat::RG8Uint:
        case wgpu::TextureFormat::RG8Sint:
            return kTwoBytes;
        case wgpu::TextureFormat::R32Float:
        case wgpu::TextureFormat::R32Uint:
        case wgpu::TextureFormat::R32Sint:
        case wgpu::TextureFormat::RG16Unorm:
        case wgpu::TextureFormat::RG16Snorm:
        case wgpu::TextureFormat::RG16Uint:
        case wgpu::TextureFormat::RG16Sint:
        case wgpu::TextureFormat::RG16Float:
        case wgpu::TextureFormat::RGBA8Unorm:
        case wgpu::TextureFormat::RGBA8UnormSrgb:
        case wgpu::TextureFormat::RGBA8Snorm:
        case wgpu::TextureFormat::RGBA8Uint:
        case wgpu::TextureFormat::RGBA8Sint:
        case wgpu::TextureFormat::BGRA8Unorm:
        case wgpu::TextureFormat::BGRA8UnormSrgb:
        case wgpu::TextureFormat::RGB10A2Uint:
        case wgpu::TextureFormat::RGB10A2Unorm:
        case wgpu::TextureFormat::RG11B10Ufloat:
        case wgpu::TextureFormat::RGB9E5Ufloat:
            return kFourBytes;
        case wgpu::TextureFormat::RG32Float:
            break;
        case wgpu::TextureFormat::RG32Uint:
            break;
        case wgpu::TextureFormat::RG32Sint:
            break;
        case wgpu::TextureFormat::RGBA16Unorm:
            break;
        case wgpu::TextureFormat::RGBA16Snorm:
            break;
        case wgpu::TextureFormat::RGBA16Uint:
            break;
        case wgpu::TextureFormat::RGBA16Sint:
            break;
        case wgpu::TextureFormat::RGBA16Float:
            break;
        case wgpu::TextureFormat::RGBA32Float:
            break;
        case wgpu::TextureFormat::RGBA32Uint:
            break;
        case wgpu::TextureFormat::RGBA32Sint:
            break;
        case wgpu::TextureFormat::Stencil8:
            break;
        case wgpu::TextureFormat::Depth16Unorm:
            break;
        case wgpu::TextureFormat::Depth24Plus:
            break;
        case wgpu::TextureFormat::Depth24PlusStencil8:
            break;
        case wgpu::TextureFormat::Depth32Float:
            break;
        case wgpu::TextureFormat::Depth32FloatStencil8:
            break;
        case wgpu::TextureFormat::BC1RGBAUnorm:
            break;
        case wgpu::TextureFormat::BC1RGBAUnormSrgb:
            break;
        case wgpu::TextureFormat::BC2RGBAUnorm:
            break;
        case wgpu::TextureFormat::BC2RGBAUnormSrgb:
            break;
        case wgpu::TextureFormat::BC3RGBAUnorm:
            break;
        case wgpu::TextureFormat::BC3RGBAUnormSrgb:
            break;
        case wgpu::TextureFormat::BC4RUnorm:
            break;
        case wgpu::TextureFormat::BC4RSnorm:
            break;
        case wgpu::TextureFormat::BC5RGUnorm:
            break;
        case wgpu::TextureFormat::BC5RGSnorm:
            break;
        case wgpu::TextureFormat::BC6HRGBUfloat:
            break;
        case wgpu::TextureFormat::BC6HRGBFloat:
            break;
        case wgpu::TextureFormat::BC7RGBAUnorm:
            break;
        case wgpu::TextureFormat::BC7RGBAUnormSrgb:
            break;
        case wgpu::TextureFormat::ETC2RGB8Unorm:
            break;
        case wgpu::TextureFormat::ETC2RGB8UnormSrgb:
            break;
        case wgpu::TextureFormat::ETC2RGB8A1Unorm:
            break;
        case wgpu::TextureFormat::ETC2RGB8A1UnormSrgb:
            break;
        case wgpu::TextureFormat::ETC2RGBA8Unorm:
            break;
        case wgpu::TextureFormat::ETC2RGBA8UnormSrgb:
            break;
        case wgpu::TextureFormat::EACR11Unorm:
            break;
        case wgpu::TextureFormat::EACR11Snorm:
            break;
        case wgpu::TextureFormat::EACRG11Unorm:
            break;
        case wgpu::TextureFormat::EACRG11Snorm:
            break;
        case wgpu::TextureFormat::ASTC4x4Unorm:
            break;
        case wgpu::TextureFormat::ASTC4x4UnormSrgb:
            break;
        case wgpu::TextureFormat::ASTC5x4Unorm:
            break;
        case wgpu::TextureFormat::ASTC5x4UnormSrgb:
            break;
        case wgpu::TextureFormat::ASTC5x5Unorm:
            break;
        case wgpu::TextureFormat::ASTC5x5UnormSrgb:
            break;
        case wgpu::TextureFormat::ASTC6x5Unorm:
            break;
        case wgpu::TextureFormat::ASTC6x5UnormSrgb:
            break;
        case wgpu::TextureFormat::ASTC6x6Unorm:
            break;
        case wgpu::TextureFormat::ASTC6x6UnormSrgb:
            break;
        case wgpu::TextureFormat::ASTC8x5Unorm:
            break;
        case wgpu::TextureFormat::ASTC8x5UnormSrgb:
            break;
        case wgpu::TextureFormat::ASTC8x6Unorm:
            break;
        case wgpu::TextureFormat::ASTC8x6UnormSrgb:
            break;
        case wgpu::TextureFormat::ASTC8x8Unorm:
            break;
        case wgpu::TextureFormat::ASTC8x8UnormSrgb:
            break;
        case wgpu::TextureFormat::ASTC10x5Unorm:
            break;
        case wgpu::TextureFormat::ASTC10x5UnormSrgb:
            break;
        case wgpu::TextureFormat::ASTC10x6Unorm:
            break;
        case wgpu::TextureFormat::ASTC10x6UnormSrgb:
            break;
        case wgpu::TextureFormat::ASTC10x8Unorm:
            break;
        case wgpu::TextureFormat::ASTC10x8UnormSrgb:
            break;
        case wgpu::TextureFormat::ASTC10x10Unorm:
            break;
        case wgpu::TextureFormat::ASTC10x10UnormSrgb:
            break;
        case wgpu::TextureFormat::ASTC12x10Unorm:
            break;
        case wgpu::TextureFormat::ASTC12x10UnormSrgb:
            break;
        case wgpu::TextureFormat::ASTC12x12Unorm:
            break;
        case wgpu::TextureFormat::ASTC12x12UnormSrgb:
            break;
        case wgpu::TextureFormat::R8BG8Biplanar420Unorm:
            break;
        case wgpu::TextureFormat::R10X6BG10X6Biplanar420Unorm:
            break;
        case wgpu::TextureFormat::R8BG8A8Triplanar420Unorm:
            break;
        case wgpu::TextureFormat::R8BG8Biplanar422Unorm:
            break;
        case wgpu::TextureFormat::R8BG8Biplanar444Unorm:
            break;
        case wgpu::TextureFormat::R10X6BG10X6Biplanar422Unorm:
            break;
        case wgpu::TextureFormat::R10X6BG10X6Biplanar444Unorm:
            break;
        case wgpu::TextureFormat::External:
            break;
        default:
            break;
    }
    return kUnKnown;
}
}  // namespace wgx