//
// Copyright(c) 2018 benikabocha.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
//

#ifndef TINYDDSLOADER_H_
#define TINYDDSLOADER_H_

#include <iostream>
#include <vector>

namespace tinyddsloader {

enum Result {
    Success,
    ErrorFileOpen,
    ErrorRead,
    ErrorMagicWord,
    ErrorSize,
    ErrorVerify,
    ErrorNotSupported,
    ErrorInvalidData,
};

class DDSFile {
public:
    static const char Magic[4];

    enum class PixelFormatFlagBits : uint32_t {
        FourCC = 0x00000004,
        RGB = 0x00000040,
        RGBA = 0x00000041,
        Luminance = 0x00020000,
        LuminanceA = 0x00020001,
        AlphaPixels = 0x00000001,
        Alpha = 0x00000002,
        Palette8 = 0x00000020,
        Palette8A = 0x00000021,
        BumpDUDV = 0x00080000
    };

    enum class DXGIFormat : uint32_t {
        Unknown = 0,
        R32G32B32A32_Typeless = 1,
        R32G32B32A32_Float = 2,
        R32G32B32A32_UInt = 3,
        R32G32B32A32_SInt = 4,
        R32G32B32_Typeless = 5,
        R32G32B32_Float = 6,
        R32G32B32_UInt = 7,
        R32G32B32_SInt = 8,
        R16G16B16A16_Typeless = 9,
        R16G16B16A16_Float = 10,
        R16G16B16A16_UNorm = 11,
        R16G16B16A16_UInt = 12,
        R16G16B16A16_SNorm = 13,
        R16G16B16A16_SInt = 14,
        R32G32_Typeless = 15,
        R32G32_Float = 16,
        R32G32_UInt = 17,
        R32G32_SInt = 18,
        R32G8X24_Typeless = 19,
        D32_Float_S8X24_UInt = 20,
        R32_Float_X8X24_Typeless = 21,
        X32_Typeless_G8X24_UInt = 22,
        R10G10B10A2_Typeless = 23,
        R10G10B10A2_UNorm = 24,
        R10G10B10A2_UInt = 25,
        R11G11B10_Float = 26,
        R8G8B8A8_Typeless = 27,
        R8G8B8A8_UNorm = 28,
        R8G8B8A8_UNorm_SRGB = 29,
        R8G8B8A8_UInt = 30,
        R8G8B8A8_SNorm = 31,
        R8G8B8A8_SInt = 32,
        R16G16_Typeless = 33,
        R16G16_Float = 34,
        R16G16_UNorm = 35,
        R16G16_UInt = 36,
        R16G16_SNorm = 37,
        R16G16_SInt = 38,
        R32_Typeless = 39,
        D32_Float = 40,
        R32_Float = 41,
        R32_UInt = 42,
        R32_SInt = 43,
        R24G8_Typeless = 44,
        D24_UNorm_S8_UInt = 45,
        R24_UNorm_X8_Typeless = 46,
        X24_Typeless_G8_UInt = 47,
        R8G8_Typeless = 48,
        R8G8_UNorm = 49,
        R8G8_UInt = 50,
        R8G8_SNorm = 51,
        R8G8_SInt = 52,
        R16_Typeless = 53,
        R16_Float = 54,
        D16_UNorm = 55,
        R16_UNorm = 56,
        R16_UInt = 57,
        R16_SNorm = 58,
        R16_SInt = 59,
        R8_Typeless = 60,
        R8_UNorm = 61,
        R8_UInt = 62,
        R8_SNorm = 63,
        R8_SInt = 64,
        A8_UNorm = 65,
        R1_UNorm = 66,
        R9G9B9E5_SHAREDEXP = 67,
        R8G8_B8G8_UNorm = 68,
        G8R8_G8B8_UNorm = 69,
        BC1_Typeless = 70,
        BC1_UNorm = 71,
        BC1_UNorm_SRGB = 72,
        BC2_Typeless = 73,
        BC2_UNorm = 74,
        BC2_UNorm_SRGB = 75,
        BC3_Typeless = 76,
        BC3_UNorm = 77,
        BC3_UNorm_SRGB = 78,
        BC4_Typeless = 79,
        BC4_UNorm = 80,
        BC4_SNorm = 81,
        BC5_Typeless = 82,
        BC5_UNorm = 83,
        BC5_SNorm = 84,
        B5G6R5_UNorm = 85,
        B5G5R5A1_UNorm = 86,
        B8G8R8A8_UNorm = 87,
        B8G8R8X8_UNorm = 88,
        R10G10B10_XR_BIAS_A2_UNorm = 89,
        B8G8R8A8_Typeless = 90,
        B8G8R8A8_UNorm_SRGB = 91,
        B8G8R8X8_Typeless = 92,
        B8G8R8X8_UNorm_SRGB = 93,
        BC6H_Typeless = 94,
        BC6H_UF16 = 95,
        BC6H_SF16 = 96,
        BC7_Typeless = 97,
        BC7_UNorm = 98,
        BC7_UNorm_SRGB = 99,
        AYUV = 100,
        Y410 = 101,
        Y416 = 102,
        NV12 = 103,
        P010 = 104,
        P016 = 105,
        YUV420_OPAQUE = 106,
        YUY2 = 107,
        Y210 = 108,
        Y216 = 109,
        NV11 = 110,
        AI44 = 111,
        IA44 = 112,
        P8 = 113,
        A8P8 = 114,
        B4G4R4A4_UNorm = 115,
        P208 = 130,
        V208 = 131,
        V408 = 132,
    };

    enum class HeaderFlagBits : uint32_t {
        Height = 0x00000002,
        Width = 0x00000004,
        Texture = 0x00001007,
        Mipmap = 0x00020000,
        Volume = 0x00800000,
        Pitch = 0x00000008,
        LinearSize = 0x00080000,
    };

    enum class HeaderCaps2FlagBits : uint32_t {
        CubemapPositiveX = 0x00000600,
        CubemapNegativeX = 0x00000a00,
        CubemapPositiveY = 0x00001200,
        CubemapNegativeY = 0x00002200,
        CubemapPositiveZ = 0x00004200,
        CubemapNegativeZ = 0x00008200,
        CubemapAllFaces = CubemapPositiveX | CubemapNegativeX |
                          CubemapPositiveY | CubemapNegativeY |
                          CubemapPositiveZ | CubemapNegativeZ,
        Volume = 0x00200000,
    };

    struct PixelFormat {
        uint32_t m_size;
        uint32_t m_flags;
        uint32_t m_fourCC;
        uint32_t m_bitCount;
        uint32_t m_RBitMask;
        uint32_t m_GBitMask;
        uint32_t m_BBitMask;
        uint32_t m_ABitMask;
    };

    struct Header {
        uint32_t m_size;
        uint32_t m_flags;
        uint32_t m_height;
        uint32_t m_width;
        uint32_t m_pitchOrLinerSize;
        uint32_t m_depth;
        uint32_t m_mipMapCount;
        uint32_t m_reserved1[11];
        PixelFormat m_pixelFormat;
        uint32_t m_caps;
        uint32_t m_caps2;
        uint32_t m_caps3;
        uint32_t m_caps4;
        uint32_t m_reserved2;
    };

    enum class TextureDimension : uint32_t {
        Unknown = 0,
        Texture1D = 2,
        Texture2D = 3,
        Texture3D = 4
    };

    enum class DXT10MiscFlagBits : uint32_t { TextureCube = 0x4 };

    struct HeaderDXT10 {
        DXGIFormat m_format;
        TextureDimension m_resourceDimension;
        uint32_t m_miscFlag;
        uint32_t m_arraySize;
        uint32_t m_miscFlag2;
    };

    struct ImageData {
        uint32_t m_width;
        uint32_t m_height;
        uint32_t m_depth;
        void* m_mem;
        uint32_t m_memPitch;
        uint32_t m_memSlicePitch;
    };

    struct BC1Block {
        uint16_t m_color0;
        uint16_t m_color1;
        uint8_t m_row0;
        uint8_t m_row1;
        uint8_t m_row2;
        uint8_t m_row3;
    };

    struct BC2Block {
        uint16_t m_alphaRow0;
        uint16_t m_alphaRow1;
        uint16_t m_alphaRow2;
        uint16_t m_alphaRow3;
        uint16_t m_color0;
        uint16_t m_color1;
        uint8_t m_row0;
        uint8_t m_row1;
        uint8_t m_row2;
        uint8_t m_row3;
    };

    struct BC3Block {
        uint8_t m_alpha0;
        uint8_t m_alpha1;
        uint8_t m_alphaR0;
        uint8_t m_alphaR1;
        uint8_t m_alphaR2;
        uint8_t m_alphaR3;
        uint8_t m_alphaR4;
        uint8_t m_alphaR5;
        uint16_t m_color0;
        uint16_t m_color1;
        uint8_t m_row0;
        uint8_t m_row1;
        uint8_t m_row2;
        uint8_t m_row3;
    };

public:
    static bool IsCompressed(DXGIFormat fmt);
    static uint32_t MakeFourCC(char ch0, char ch1, char ch2, char ch3);
    static DXGIFormat GetDXGIFormat(const PixelFormat& pf);
    static uint32_t GetBitsPerPixel(DXGIFormat fmt);

    Result Load(const char* filepath);
    Result Load(std::istream& input);
    Result Load(const uint8_t* data, size_t size);
    Result Load(std::vector<uint8_t>&& dds);

    const ImageData* GetImageData(uint32_t mipIdx = 0,
                                  uint32_t arrayIdx = 0) const {
        if (mipIdx < m_mipCount && arrayIdx < m_arraySize) {
            return &m_imageDatas[m_mipCount * arrayIdx + mipIdx];
        }
        return nullptr;
    }

    bool Flip();

    uint32_t GetWidth() const { return m_width; }
    uint32_t GetHeight() const { return m_height; }
    uint32_t GetDepth() const { return m_depth; }
    uint32_t GetMipCount() const { return m_mipCount; }
    uint32_t GetArraySize() const { return m_arraySize; }
    DXGIFormat GetFormat() const { return m_format; }
    bool IsCubemap() const { return m_isCubemap; }
    TextureDimension GetTextureDimension() const { return m_texDim; }

private:
    void GetImageInfo(uint32_t w, uint32_t h, DXGIFormat fmt,
                      uint32_t* outNumBytes, uint32_t* outRowBytes,
                      uint32_t* outNumRows);
    bool FlipImage(ImageData& imageData);
    bool FlipCompressedImage(ImageData& imageData);

private:
    std::vector<uint8_t> m_dds;
    std::vector<ImageData> m_imageDatas;

    uint32_t m_height;
    uint32_t m_width;
    uint32_t m_depth;
    uint32_t m_mipCount;
    uint32_t m_arraySize;
    DXGIFormat m_format;
    bool m_isCubemap;
    TextureDimension m_texDim;
};

}  // namespace tinyddsloader

#ifdef TINYDDSLOADER_IMPLEMENTATION

#if _WIN32
#undef min
#undef max
#endif  // _Win32

#include <algorithm>
#include <fstream>

namespace tinyddsloader {

const char DDSFile::Magic[4] = {'D', 'D', 'S', ' '};

bool DDSFile::IsCompressed(DXGIFormat fmt) {
    switch (fmt) {
        case DXGIFormat::BC1_Typeless:
        case DXGIFormat::BC1_UNorm:
        case DXGIFormat::BC1_UNorm_SRGB:
        case DXGIFormat::BC2_Typeless:
        case DXGIFormat::BC2_UNorm:
        case DXGIFormat::BC2_UNorm_SRGB:
        case DXGIFormat::BC3_Typeless:
        case DXGIFormat::BC3_UNorm:
        case DXGIFormat::BC3_UNorm_SRGB:
        case DXGIFormat::BC4_Typeless:
        case DXGIFormat::BC4_UNorm:
        case DXGIFormat::BC4_SNorm:
        case DXGIFormat::BC5_Typeless:
        case DXGIFormat::BC5_UNorm:
        case DXGIFormat::BC5_SNorm:
        case DXGIFormat::BC6H_Typeless:
        case DXGIFormat::BC6H_UF16:
        case DXGIFormat::BC6H_SF16:
        case DXGIFormat::BC7_Typeless:
        case DXGIFormat::BC7_UNorm:
        case DXGIFormat::BC7_UNorm_SRGB:
            return true;
    }
    return false;
}

uint32_t DDSFile::MakeFourCC(char ch0, char ch1, char ch2, char ch3) {
    return (uint32_t(uint8_t(ch0)) | (uint32_t(uint8_t(ch1)) << 8) |
            (uint32_t(uint8_t(ch2)) << 16) | (uint32_t(uint8_t(ch3)) << 24));
}

DDSFile::DXGIFormat DDSFile::GetDXGIFormat(const PixelFormat& pf) {
    if (pf.m_flags & uint32_t(PixelFormatFlagBits::RGB)) {
        switch (pf.m_bitCount) {
            case 32:
                if (pf.m_RBitMask == 0x000000ff &&
                    pf.m_GBitMask == 0x0000ff00 &&
                    pf.m_BBitMask == 0x00ff0000 &&
                    pf.m_ABitMask == 0xff000000) {
                    return DXGIFormat::R8G8B8A8_UNorm;
                }
                if (pf.m_RBitMask == 0x00ff0000 &&
                    pf.m_GBitMask == 0x0000ff00 &&
                    pf.m_BBitMask == 0x000000ff &&
                    pf.m_ABitMask == 0xff000000) {
                    return DXGIFormat::B8G8R8A8_UNorm;
                }
                if (pf.m_RBitMask == 0x00ff0000 &&
                    pf.m_GBitMask == 0x0000ff00 &&
                    pf.m_BBitMask == 0x000000ff &&
                    pf.m_ABitMask == 0x00000000) {
                    return DXGIFormat::B8G8R8X8_UNorm;
                }

                if (pf.m_RBitMask == 0x0000ffff &&
                    pf.m_GBitMask == 0xffff0000 &&
                    pf.m_BBitMask == 0x00000000 &&
                    pf.m_ABitMask == 0x00000000) {
                    return DXGIFormat::R16G16_UNorm;
                }

                if (pf.m_RBitMask == 0xffffffff &&
                    pf.m_GBitMask == 0x00000000 &&
                    pf.m_BBitMask == 0x00000000 &&
                    pf.m_ABitMask == 0x00000000) {
                    return DXGIFormat::R32_Float;
                }
                break;
            case 24:
                break;
            case 16:
                if (pf.m_RBitMask == 0x7c00 && pf.m_GBitMask == 0x03e0 &&
                    pf.m_BBitMask == 0x001f && pf.m_ABitMask == 0x8000) {
                    return DXGIFormat::B5G5R5A1_UNorm;
                }
                if (pf.m_RBitMask == 0xf800 && pf.m_GBitMask == 0x07e0 &&
                    pf.m_BBitMask == 0x001f && pf.m_ABitMask == 0x0000) {
                    return DXGIFormat::B5G6R5_UNorm;
                }

                if (pf.m_RBitMask == 0x0f00 && pf.m_GBitMask == 0x00f0 &&
                    pf.m_BBitMask == 0x000f && pf.m_ABitMask == 0xf000) {
                    return DXGIFormat::B4G4R4A4_UNorm;
                }
                break;
            default:
                break;
        }
    } else if (pf.m_flags & uint32_t(PixelFormatFlagBits::Luminance)) {
        if (8 == pf.m_bitCount) {
            if (pf.m_RBitMask == 0x000000ff && pf.m_GBitMask == 0x00000000 &&
                pf.m_BBitMask == 0x00000000 && pf.m_ABitMask == 0x00000000) {
                return DXGIFormat::R8_UNorm;
            }
            if (pf.m_RBitMask == 0x000000ff && pf.m_GBitMask == 0x0000ff00 &&
                pf.m_BBitMask == 0x00000000 && pf.m_ABitMask == 0x00000000) {
                return DXGIFormat::R8G8_UNorm;
            }
        }
        if (16 == pf.m_bitCount) {
            if (pf.m_RBitMask == 0x0000ffff && pf.m_GBitMask == 0x00000000 &&
                pf.m_BBitMask == 0x00000000 && pf.m_ABitMask == 0x00000000) {
                return DXGIFormat::R16_UNorm;
            }
            if (pf.m_RBitMask == 0x000000ff && pf.m_GBitMask == 0x0000ff00 &&
                pf.m_BBitMask == 0x00000000 && pf.m_ABitMask == 0x00000000) {
                return DXGIFormat::R8G8_UNorm;
            }
        }
    } else if (pf.m_flags & uint32_t(PixelFormatFlagBits::Alpha)) {
        if (8 == pf.m_bitCount) {
            return DXGIFormat::A8_UNorm;
        }
    } else if (pf.m_flags & uint32_t(PixelFormatFlagBits::BumpDUDV)) {
        if (16 == pf.m_bitCount) {
            if (pf.m_RBitMask == 0x00ff && pf.m_GBitMask == 0xff00 &&
                pf.m_BBitMask == 0x0000 && pf.m_ABitMask == 0x0000) {
                return DXGIFormat::R8G8_SNorm;
            }
        }
        if (32 == pf.m_bitCount) {
            if (pf.m_RBitMask == 0x000000ff && pf.m_GBitMask == 0x0000ff00 &&
                pf.m_BBitMask == 0x00ff0000 && pf.m_ABitMask == 0xff000000) {
                return DXGIFormat::R8G8B8A8_SNorm;
            }
            if (pf.m_RBitMask == 0x0000ffff && pf.m_GBitMask == 0xffff0000 &&
                pf.m_BBitMask == 0x00000000 && pf.m_ABitMask == 0x00000000) {
                return DXGIFormat::R16G16_SNorm;
            }
        }
    } else if (pf.m_flags & uint32_t(PixelFormatFlagBits::FourCC)) {
        if (MakeFourCC('D', 'X', 'T', '1') == pf.m_fourCC) {
            return DXGIFormat::BC1_UNorm;
        }
        if (MakeFourCC('D', 'X', 'T', '3') == pf.m_fourCC) {
            return DXGIFormat::BC2_UNorm;
        }
        if (MakeFourCC('D', 'X', 'T', '5') == pf.m_fourCC) {
            return DXGIFormat::BC3_UNorm;
        }

        if (MakeFourCC('D', 'X', 'T', '4') == pf.m_fourCC) {
            return DXGIFormat::BC2_UNorm;
        }
        if (MakeFourCC('D', 'X', 'T', '5') == pf.m_fourCC) {
            return DXGIFormat::BC3_UNorm;
        }

        if (MakeFourCC('A', 'T', 'I', '1') == pf.m_fourCC) {
            return DXGIFormat::BC4_UNorm;
        }
        if (MakeFourCC('B', 'C', '4', 'U') == pf.m_fourCC) {
            return DXGIFormat::BC4_UNorm;
        }
        if (MakeFourCC('B', 'C', '4', 'S') == pf.m_fourCC) {
            return DXGIFormat::BC4_SNorm;
        }

        if (MakeFourCC('A', 'T', 'I', '2') == pf.m_fourCC) {
            return DXGIFormat::BC5_UNorm;
        }
        if (MakeFourCC('B', 'C', '5', 'U') == pf.m_fourCC) {
            return DXGIFormat::BC5_UNorm;
        }
        if (MakeFourCC('B', 'C', '5', 'S') == pf.m_fourCC) {
            return DXGIFormat::BC5_SNorm;
        }

        if (MakeFourCC('R', 'G', 'B', 'G') == pf.m_fourCC) {
            return DXGIFormat::R8G8_B8G8_UNorm;
        }
        if (MakeFourCC('G', 'R', 'G', 'B') == pf.m_fourCC) {
            return DXGIFormat::G8R8_G8B8_UNorm;
        }

        if (MakeFourCC('Y', 'U', 'Y', '2') == pf.m_fourCC) {
            return DXGIFormat::YUY2;
        }

        switch (pf.m_fourCC) {
            case 36:
                return DXGIFormat::R16G16B16A16_UNorm;
            case 110:
                return DXGIFormat::R16G16B16A16_SNorm;
            case 111:
                return DXGIFormat::R16_Float;
            case 112:
                return DXGIFormat::R16G16_Float;
            case 113:
                return DXGIFormat::R16G16B16A16_Float;
            case 114:
                return DXGIFormat::R32_Float;
            case 115:
                return DXGIFormat::R32G32_Float;
            case 116:
                return DXGIFormat::R32G32B32A32_Float;
        }
    }

    return DXGIFormat::Unknown;
}

uint32_t DDSFile::GetBitsPerPixel(DXGIFormat fmt) {
    switch (fmt) {
        case DXGIFormat::R32G32B32A32_Typeless:
        case DXGIFormat::R32G32B32A32_Float:
        case DXGIFormat::R32G32B32A32_UInt:
        case DXGIFormat::R32G32B32A32_SInt:
            return 128;

        case DXGIFormat::R32G32B32_Typeless:
        case DXGIFormat::R32G32B32_Float:
        case DXGIFormat::R32G32B32_UInt:
        case DXGIFormat::R32G32B32_SInt:
            return 96;

        case DXGIFormat::R16G16B16A16_Typeless:
        case DXGIFormat::R16G16B16A16_Float:
        case DXGIFormat::R16G16B16A16_UNorm:
        case DXGIFormat::R16G16B16A16_UInt:
        case DXGIFormat::R16G16B16A16_SNorm:
        case DXGIFormat::R16G16B16A16_SInt:
        case DXGIFormat::R32G32_Typeless:
        case DXGIFormat::R32G32_Float:
        case DXGIFormat::R32G32_UInt:
        case DXGIFormat::R32G32_SInt:
        case DXGIFormat::R32G8X24_Typeless:
        case DXGIFormat::D32_Float_S8X24_UInt:
        case DXGIFormat::R32_Float_X8X24_Typeless:
        case DXGIFormat::X32_Typeless_G8X24_UInt:
        case DXGIFormat::Y416:
        case DXGIFormat::Y210:
        case DXGIFormat::Y216:
            return 64;

        case DXGIFormat::R10G10B10A2_Typeless:
        case DXGIFormat::R10G10B10A2_UNorm:
        case DXGIFormat::R10G10B10A2_UInt:
        case DXGIFormat::R11G11B10_Float:
        case DXGIFormat::R8G8B8A8_Typeless:
        case DXGIFormat::R8G8B8A8_UNorm:
        case DXGIFormat::R8G8B8A8_UNorm_SRGB:
        case DXGIFormat::R8G8B8A8_UInt:
        case DXGIFormat::R8G8B8A8_SNorm:
        case DXGIFormat::R8G8B8A8_SInt:
        case DXGIFormat::R16G16_Typeless:
        case DXGIFormat::R16G16_Float:
        case DXGIFormat::R16G16_UNorm:
        case DXGIFormat::R16G16_UInt:
        case DXGIFormat::R16G16_SNorm:
        case DXGIFormat::R16G16_SInt:
        case DXGIFormat::R32_Typeless:
        case DXGIFormat::D32_Float:
        case DXGIFormat::R32_Float:
        case DXGIFormat::R32_UInt:
        case DXGIFormat::R32_SInt:
        case DXGIFormat::R24G8_Typeless:
        case DXGIFormat::D24_UNorm_S8_UInt:
        case DXGIFormat::R24_UNorm_X8_Typeless:
        case DXGIFormat::X24_Typeless_G8_UInt:
        case DXGIFormat::R9G9B9E5_SHAREDEXP:
        case DXGIFormat::R8G8_B8G8_UNorm:
        case DXGIFormat::G8R8_G8B8_UNorm:
        case DXGIFormat::B8G8R8A8_UNorm:
        case DXGIFormat::B8G8R8X8_UNorm:
        case DXGIFormat::R10G10B10_XR_BIAS_A2_UNorm:
        case DXGIFormat::B8G8R8A8_Typeless:
        case DXGIFormat::B8G8R8A8_UNorm_SRGB:
        case DXGIFormat::B8G8R8X8_Typeless:
        case DXGIFormat::B8G8R8X8_UNorm_SRGB:
        case DXGIFormat::AYUV:
        case DXGIFormat::Y410:
        case DXGIFormat::YUY2:
            return 32;

        case DXGIFormat::P010:
        case DXGIFormat::P016:
            return 24;

        case DXGIFormat::R8G8_Typeless:
        case DXGIFormat::R8G8_UNorm:
        case DXGIFormat::R8G8_UInt:
        case DXGIFormat::R8G8_SNorm:
        case DXGIFormat::R8G8_SInt:
        case DXGIFormat::R16_Typeless:
        case DXGIFormat::R16_Float:
        case DXGIFormat::D16_UNorm:
        case DXGIFormat::R16_UNorm:
        case DXGIFormat::R16_UInt:
        case DXGIFormat::R16_SNorm:
        case DXGIFormat::R16_SInt:
        case DXGIFormat::B5G6R5_UNorm:
        case DXGIFormat::B5G5R5A1_UNorm:
        case DXGIFormat::A8P8:
        case DXGIFormat::B4G4R4A4_UNorm:
            return 16;

        case DXGIFormat::NV12:
        case DXGIFormat::YUV420_OPAQUE:
        case DXGIFormat::NV11:
            return 12;

        case DXGIFormat::R8_Typeless:
        case DXGIFormat::R8_UNorm:
        case DXGIFormat::R8_UInt:
        case DXGIFormat::R8_SNorm:
        case DXGIFormat::R8_SInt:
        case DXGIFormat::A8_UNorm:
        case DXGIFormat::AI44:
        case DXGIFormat::IA44:
        case DXGIFormat::P8:
            return 8;

        case DXGIFormat::R1_UNorm:
            return 1;

        case DXGIFormat::BC1_Typeless:
        case DXGIFormat::BC1_UNorm:
        case DXGIFormat::BC1_UNorm_SRGB:
        case DXGIFormat::BC4_Typeless:
        case DXGIFormat::BC4_UNorm:
        case DXGIFormat::BC4_SNorm:
            return 4;

        case DXGIFormat::BC2_Typeless:
        case DXGIFormat::BC2_UNorm:
        case DXGIFormat::BC2_UNorm_SRGB:
        case DXGIFormat::BC3_Typeless:
        case DXGIFormat::BC3_UNorm:
        case DXGIFormat::BC3_UNorm_SRGB:
        case DXGIFormat::BC5_Typeless:
        case DXGIFormat::BC5_UNorm:
        case DXGIFormat::BC5_SNorm:
        case DXGIFormat::BC6H_Typeless:
        case DXGIFormat::BC6H_UF16:
        case DXGIFormat::BC6H_SF16:
        case DXGIFormat::BC7_Typeless:
        case DXGIFormat::BC7_UNorm:
        case DXGIFormat::BC7_UNorm_SRGB:
            return 8;
        default:
            return 0;
    }
}

Result DDSFile::Load(const char* filepath) {
    std::ifstream ifs(filepath, std::ios_base::binary);
    if (!ifs.is_open()) {
        return Result::ErrorFileOpen;
    }

    return Load(ifs);
}

Result DDSFile::Load(std::istream& input) {
    m_dds.clear();

    input.seekg(0, std::ios_base::beg);
    auto begPos = input.tellg();
    input.seekg(0, std::ios_base::end);
    auto endPos = input.tellg();
    input.seekg(0, std::ios_base::beg);

    auto fileSize = endPos - begPos;
    if (fileSize == 0) {
        return Result::ErrorRead;
    }
    std::vector<uint8_t> dds((size_t)fileSize);

    input.read(reinterpret_cast<char*>(dds.data()), fileSize);

    if (input.bad()) {
        return Result::ErrorRead;
    }

	return Load(std::move(dds));
}

Result DDSFile::Load(const uint8_t* data, size_t size) {
    std::vector<uint8_t> dds(data, data + size);
    return Load(std::move(dds));
}

Result DDSFile::Load(std::vector<uint8_t>&& dds) {
    m_dds.clear();

    if (dds.size() < 4) {
        return Result::ErrorSize;
    }

    for (int i = 0; i < 4; i++) {
        if (dds[i] != Magic[i]) {
            return Result::ErrorMagicWord;
        }
    }

    if ((sizeof(uint32_t) + sizeof(Header)) >= dds.size()) {
        return Result::ErrorSize;
    }
    auto header =
        reinterpret_cast<const Header*>(dds.data() + sizeof(uint32_t));

    if (header->m_size != sizeof(Header) ||
        header->m_pixelFormat.m_size != sizeof(PixelFormat)) {
        return Result::ErrorVerify;
    }
    bool dxt10Header = false;
    if ((header->m_pixelFormat.m_flags &
         uint32_t(PixelFormatFlagBits::FourCC)) &&
        (MakeFourCC('D', 'X', '1', '0') == header->m_pixelFormat.m_fourCC)) {
        if ((sizeof(uint32_t) + sizeof(Header) + sizeof(HeaderDXT10)) >=
            dds.size()) {
            return Result::ErrorSize;
        }
        dxt10Header = true;
    }
    ptrdiff_t offset = sizeof(uint32_t) + sizeof(Header) +
                       (dxt10Header ? sizeof(HeaderDXT10) : 0);

    m_height = header->m_height;
    m_width = header->m_width;
    m_texDim = TextureDimension::Unknown;
    m_arraySize = 1;
    m_format = DXGIFormat::Unknown;
    m_isCubemap = false;
    m_mipCount = header->m_mipMapCount;
    if (0 == m_mipCount) {
        m_mipCount = 1;
    }

    if (dxt10Header) {
        auto dxt10Header = reinterpret_cast<const HeaderDXT10*>(
            reinterpret_cast<const char*>(header) + sizeof(Header));

        m_arraySize = dxt10Header->m_arraySize;
        if (m_arraySize == 0) {
            return Result::ErrorInvalidData;
        }

        switch (dxt10Header->m_format) {
            case DXGIFormat::AI44:
            case DXGIFormat::IA44:
            case DXGIFormat::P8:
            case DXGIFormat::A8P8:
                return Result::ErrorNotSupported;
            default:
                if (GetBitsPerPixel(dxt10Header->m_format) == 0) {
                    return Result::ErrorNotSupported;
                }
        }

        m_format = dxt10Header->m_format;

        switch (dxt10Header->m_resourceDimension) {
            case TextureDimension::Texture1D:
                if ((header->m_flags & uint32_t(HeaderFlagBits::Height) &&
                     (m_height != 1))) {
                    return Result::ErrorInvalidData;
                }
                m_height = m_depth = 1;
                break;
            case TextureDimension::Texture2D:
                if (dxt10Header->m_miscFlag &
                    uint32_t(DXT10MiscFlagBits::TextureCube)) {
                    m_arraySize *= 6;
                    m_isCubemap = true;
                }
                m_depth = 1;
                break;
            case TextureDimension::Texture3D:
                if (!(header->m_flags & uint32_t(HeaderFlagBits::Volume))) {
                    return Result::ErrorInvalidData;
                }
                if (m_arraySize > 1) {
                    return Result::ErrorNotSupported;
                }
                break;
            default:
                return Result::ErrorNotSupported;
        }

        m_texDim = dxt10Header->m_resourceDimension;
    } else {
        m_format = GetDXGIFormat(header->m_pixelFormat);
        if (m_format == DXGIFormat::Unknown) {
            return Result::ErrorNotSupported;
        }

        if (header->m_flags & uint32_t(HeaderFlagBits::Volume)) {
            m_texDim = TextureDimension::Texture3D;
        } else {
			auto caps2 = header->m_caps2 &
				uint32_t(HeaderCaps2FlagBits::CubemapAllFaces);
            if (caps2) {
                if (caps2 != uint32_t(HeaderCaps2FlagBits::CubemapAllFaces)) {
                    return Result::ErrorNotSupported;
                }
                m_arraySize = 6;
                m_isCubemap = true;
            }

            m_depth = 1;
            m_texDim = TextureDimension::Texture2D;
        }
    }

    std::vector<ImageData> imageDatas(m_mipCount * m_arraySize);
    uint8_t* srcBits = dds.data() + offset;
    uint8_t* endBits = dds.data() + dds.size();
    uint32_t idx = 0;
    for (uint32_t j = 0; j < m_arraySize; j++) {
        uint32_t w = m_width;
        uint32_t h = m_height;
        uint32_t d = m_depth;
        for (uint32_t i = 0; i < m_mipCount; i++) {
            uint32_t numBytes;
            uint32_t rowBytes;
            GetImageInfo(w, h, m_format, &numBytes, &rowBytes, nullptr);

            imageDatas[idx].m_width = w;
            imageDatas[idx].m_height = h;
            imageDatas[idx].m_depth = d;
            imageDatas[idx].m_mem = srcBits;
            imageDatas[idx].m_memPitch = rowBytes;
            imageDatas[idx].m_memSlicePitch = numBytes;
            idx++;

            if (srcBits + (numBytes * d) > endBits) {
                return Result::ErrorInvalidData;
            }
            srcBits += numBytes * d;
            w = std::max<uint32_t>(1, w / 2);
            h = std::max<uint32_t>(1, h / 2);
            d = std::max<uint32_t>(1, d / 2);
        }
    }

    m_dds = std::move(dds);
    m_imageDatas = std::move(imageDatas);

    return Result::Success;
}

void DDSFile::GetImageInfo(uint32_t w, uint32_t h, DXGIFormat fmt,
                           uint32_t* outNumBytes, uint32_t* outRowBytes,
                           uint32_t* outNumRows) {
    uint32_t numBytes = 0;
    uint32_t rowBytes = 0;
    uint32_t numRows = 0;

    bool bc = false;
    bool packed = false;
    bool planar = false;
    uint32_t bpe = 0;
    switch (fmt) {
        case DXGIFormat::BC1_Typeless:
        case DXGIFormat::BC1_UNorm:
        case DXGIFormat::BC1_UNorm_SRGB:
        case DXGIFormat::BC4_Typeless:
        case DXGIFormat::BC4_UNorm:
        case DXGIFormat::BC4_SNorm:
            bc = true;
            bpe = 8;
            break;

        case DXGIFormat::BC2_Typeless:
        case DXGIFormat::BC2_UNorm:
        case DXGIFormat::BC2_UNorm_SRGB:
        case DXGIFormat::BC3_Typeless:
        case DXGIFormat::BC3_UNorm:
        case DXGIFormat::BC3_UNorm_SRGB:
        case DXGIFormat::BC5_Typeless:
        case DXGIFormat::BC5_UNorm:
        case DXGIFormat::BC5_SNorm:
        case DXGIFormat::BC6H_Typeless:
        case DXGIFormat::BC6H_UF16:
        case DXGIFormat::BC6H_SF16:
        case DXGIFormat::BC7_Typeless:
        case DXGIFormat::BC7_UNorm:
        case DXGIFormat::BC7_UNorm_SRGB:
            bc = true;
            bpe = 16;
            break;

        case DXGIFormat::R8G8_B8G8_UNorm:
        case DXGIFormat::G8R8_G8B8_UNorm:
        case DXGIFormat::YUY2:
            packed = true;
            bpe = 4;
            break;

        case DXGIFormat::Y210:
        case DXGIFormat::Y216:
            packed = true;
            bpe = 8;
            break;

        case DXGIFormat::NV12:
        case DXGIFormat::YUV420_OPAQUE:
            planar = true;
            bpe = 2;
            break;

        case DXGIFormat::P010:
        case DXGIFormat::P016:
            planar = true;
            bpe = 4;
            break;
        default:
            break;
    }

    if (bc) {
        uint32_t numBlocksWide = 0;
        if (w > 0) {
            numBlocksWide = std::max<uint32_t>(1, (w + 3) / 4);
        }
        uint32_t numBlocksHigh = 0;
        if (h > 0) {
            numBlocksHigh = std::max<uint32_t>(1, (h + 3) / 4);
        }
        rowBytes = numBlocksWide * bpe;
        numRows = numBlocksHigh;
        numBytes = rowBytes * numBlocksHigh;
    } else if (packed) {
        rowBytes = ((w + 1) >> 1) * bpe;
        numRows = h;
        numBytes = rowBytes * h;
    } else if (fmt == DXGIFormat::NV11) {
        rowBytes = ((w + 3) >> 2) * 4;
        numRows = h * 2;
        numBytes = rowBytes + numRows;
    } else if (planar) {
        rowBytes = ((w + 1) >> 1) * bpe;
        numBytes = (rowBytes * h) + ((rowBytes * h + 1) >> 1);
        numRows = h + ((h + 1) >> 1);
    } else {
        uint32_t bpp = GetBitsPerPixel(fmt);
        rowBytes = (w * bpp + 7) / 8;
        numRows = h;
        numBytes = rowBytes * h;
    }

    if (outNumBytes) {
        *outNumBytes = numBytes;
    }
    if (outRowBytes) {
        *outRowBytes = rowBytes;
    }
    if (outNumRows) {
        *outNumRows = numRows;
    }
}

bool DDSFile::Flip() {
    if (IsCompressed(m_format)) {
        for (auto& imageData : m_imageDatas) {
            if (!FlipCompressedImage(imageData)) {
                return false;
            }
        }
    } else {
        for (auto& imageData : m_imageDatas) {
            if (!FlipImage(imageData)) {
                return false;
            }
        }
    }
    return true;
}

bool DDSFile::FlipImage(ImageData& imageData) {
    for (uint32_t y = 0; y < imageData.m_height / 2; y++) {
        auto line0 = (uint8_t*)imageData.m_mem + y * imageData.m_memPitch;
        auto line1 = (uint8_t*)imageData.m_mem +
                     (imageData.m_height - y - 1) * imageData.m_memPitch;
        for (uint32_t i = 0; i < imageData.m_memPitch; i++) {
            std::swap(*line0, *line1);
            line0++;
            line1++;
        }
    }
    return true;
}

bool DDSFile::FlipCompressedImage(ImageData& imageData) {
    if (DXGIFormat::BC1_Typeless == m_format ||
        DXGIFormat::BC1_UNorm == m_format ||
        DXGIFormat::BC1_UNorm_SRGB == m_format) {
        uint32_t numXBlocks = (imageData.m_width + 3) / 4;
        uint32_t numYBlocks = (imageData.m_height + 3) / 4;
        if (imageData.m_height == 1) {
            return true;
        } else if (imageData.m_height == 2) {
            auto blocks = (BC1Block*)imageData.m_mem;
            for (uint32_t x = 0; x < numXBlocks; x++) {
                auto block = blocks + x;
                std::swap(block->m_row0, block->m_row1);
                std::swap(block->m_row2, block->m_row3);
            }
            return true;
        } else {
            for (uint32_t y = 0; y < (numYBlocks + 1) / 2; y++) {
                auto blocks0 = (BC1Block*)((uint8_t*)imageData.m_mem +
                                           imageData.m_memPitch * y);
                auto blocks1 =
                    (BC1Block*)((uint8_t*)imageData.m_mem +
                                imageData.m_memPitch * (numYBlocks - y - 1));
                for (uint32_t x = 0; x < numXBlocks; x++) {
                    auto block0 = blocks0 + x;
                    auto block1 = blocks1 + x;
                    std::swap(block0->m_color0, block1->m_color0);
                    std::swap(block0->m_color1, block1->m_color1);
                    std::swap(block0->m_row0, block1->m_row3);
                    std::swap(block0->m_row1, block1->m_row2);
                    std::swap(block0->m_row2, block1->m_row1);
                    std::swap(block0->m_row3, block1->m_row0);
                }
            }
            return true;
        }
    } else if (DXGIFormat::BC2_Typeless == m_format ||
               DXGIFormat::BC2_UNorm == m_format ||
               DXGIFormat::BC2_UNorm_SRGB == m_format) {
        uint32_t numXBlocks = (imageData.m_width + 3) / 4;
        uint32_t numYBlocks = (imageData.m_height + 3) / 4;
        if (imageData.m_height == 1) {
            return true;
        } else if (imageData.m_height == 2) {
            auto blocks = (BC2Block*)imageData.m_mem;
            for (uint32_t x = 0; x < numXBlocks; x++) {
                auto block = blocks + x;
                std::swap(block->m_alphaRow0, block->m_alphaRow1);
                std::swap(block->m_alphaRow2, block->m_alphaRow3);
                std::swap(block->m_row0, block->m_row1);
                std::swap(block->m_row2, block->m_row3);
            }
            return true;
        } else {
            for (uint32_t y = 0; y < (numYBlocks + 1) / 2; y++) {
                auto blocks0 = (BC2Block*)((uint8_t*)imageData.m_mem +
                                           imageData.m_memPitch * y);
                auto blocks1 =
                    (BC2Block*)((uint8_t*)imageData.m_mem +
                                imageData.m_memPitch * (numYBlocks - y - 1));
                for (uint32_t x = 0; x < numXBlocks; x++) {
                    auto block0 = blocks0 + x;
                    auto block1 = blocks1 + x;
                    std::swap(block0->m_alphaRow0, block1->m_alphaRow3);
                    std::swap(block0->m_alphaRow1, block1->m_alphaRow2);
                    std::swap(block0->m_alphaRow2, block1->m_alphaRow1);
                    std::swap(block0->m_alphaRow3, block1->m_alphaRow0);
                    std::swap(block0->m_color0, block1->m_color0);
                    std::swap(block0->m_color1, block1->m_color1);
                    std::swap(block0->m_row0, block1->m_row3);
                    std::swap(block0->m_row1, block1->m_row2);
                    std::swap(block0->m_row2, block1->m_row1);
                    std::swap(block0->m_row3, block1->m_row0);
                }
            }
            return true;
        }
    } else if (DXGIFormat::BC3_Typeless == m_format ||
               DXGIFormat::BC3_UNorm == m_format ||
               DXGIFormat::BC3_UNorm_SRGB == m_format) {
        uint32_t numXBlocks = (imageData.m_width + 3) / 4;
        uint32_t numYBlocks = (imageData.m_height + 3) / 4;
        if (imageData.m_height == 1) {
            return true;
        } else if (imageData.m_height == 2) {
            auto blocks = (BC3Block*)imageData.m_mem;
            for (uint32_t x = 0; x < numXBlocks; x++) {
                auto block = blocks + x;
                uint8_t r0 = (block->m_alphaR1 >> 4) | (block->m_alphaR2 << 4);
                uint8_t r1 = (block->m_alphaR2 >> 4) | (block->m_alphaR0 << 4);
                uint8_t r2 = (block->m_alphaR0 >> 4) | (block->m_alphaR1 << 4);
                uint8_t r3 = (block->m_alphaR4 >> 4) | (block->m_alphaR5 << 4);
                uint8_t r4 = (block->m_alphaR5 >> 4) | (block->m_alphaR3 << 4);
                uint8_t r5 = (block->m_alphaR3 >> 4) | (block->m_alphaR4 << 4);

                block->m_alphaR0 = r0;
                block->m_alphaR1 = r1;
                block->m_alphaR2 = r2;
                block->m_alphaR3 = r3;
                block->m_alphaR4 = r4;
                block->m_alphaR5 = r5;
                std::swap(block->m_row0, block->m_row1);
                std::swap(block->m_row2, block->m_row3);
            }
            return true;
        } else {
            for (uint32_t y = 0; y < (numYBlocks + 1) / 2; y++) {
                auto blocks0 = (BC3Block*)((uint8_t*)imageData.m_mem +
                                           imageData.m_memPitch * y);
                auto blocks1 =
                    (BC3Block*)((uint8_t*)imageData.m_mem +
                                imageData.m_memPitch * (numYBlocks - y - 1));
                for (uint32_t x = 0; x < numXBlocks; x++) {
                    auto block0 = blocks0 + x;
                    auto block1 = blocks1 + x;
                    std::swap(block0->m_alpha0, block1->m_alpha0);
                    std::swap(block0->m_alpha1, block1->m_alpha1);

                    uint8_t r0[6];
                    r0[0] = (block0->m_alphaR4 >> 4) | (block0->m_alphaR5 << 4);
                    r0[1] = (block0->m_alphaR5 >> 4) | (block0->m_alphaR3 << 4);
                    r0[2] = (block0->m_alphaR3 >> 4) | (block0->m_alphaR4 << 4);
                    r0[3] = (block0->m_alphaR1 >> 4) | (block0->m_alphaR2 << 4);
                    r0[4] = (block0->m_alphaR2 >> 4) | (block0->m_alphaR0 << 4);
                    r0[5] = (block0->m_alphaR0 >> 4) | (block0->m_alphaR1 << 4);
                    uint8_t r1[6];
                    r1[0] = (block1->m_alphaR4 >> 4) | (block1->m_alphaR5 << 4);
                    r1[1] = (block1->m_alphaR5 >> 4) | (block1->m_alphaR3 << 4);
                    r1[2] = (block1->m_alphaR3 >> 4) | (block1->m_alphaR4 << 4);
                    r1[3] = (block1->m_alphaR1 >> 4) | (block1->m_alphaR2 << 4);
                    r1[4] = (block1->m_alphaR2 >> 4) | (block1->m_alphaR0 << 4);
                    r1[5] = (block1->m_alphaR0 >> 4) | (block1->m_alphaR1 << 4);

                    block0->m_alphaR0 = r1[0];
                    block0->m_alphaR1 = r1[1];
                    block0->m_alphaR2 = r1[2];
                    block0->m_alphaR3 = r1[3];
                    block0->m_alphaR4 = r1[4];
                    block0->m_alphaR5 = r1[5];

                    block1->m_alphaR0 = r0[0];
                    block1->m_alphaR1 = r0[1];
                    block1->m_alphaR2 = r0[2];
                    block1->m_alphaR3 = r0[3];
                    block1->m_alphaR4 = r0[4];
                    block1->m_alphaR5 = r0[5];

                    std::swap(block0->m_color0, block1->m_color0);
                    std::swap(block0->m_color1, block1->m_color1);
                    std::swap(block0->m_row0, block1->m_row3);
                    std::swap(block0->m_row1, block1->m_row2);
                    std::swap(block0->m_row2, block1->m_row1);
                    std::swap(block0->m_row3, block1->m_row0);
                }
            }
            return true;
        }
    }
    return false;
}

}  // namespace tinyddsloader

#endif  // !TINYDDSLOADER_IMPLEMENTATION

#endif  // !TINYDDSLOADER_H_
