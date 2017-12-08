//--------------------------------------------------------------------------------------
// File: ScreenGrab12.cpp
//
// Function for capturing a 2D texture and saving it to a file (aka a 'screenshot'
// when used on a Direct3D 12 Render Target).
//
// Note these functions are useful as a light-weight runtime screen grabber. For
// full-featured texture capture, DDS writer, and texture processing pipeline,
// see the 'Texconv' sample and the 'DirectXTex' library.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// http://go.microsoft.com/fwlink/?LinkId=248926
// http://go.microsoft.com/fwlink/?LinkID=615561
//--------------------------------------------------------------------------------------

// Does not capture 1D textures or 3D textures (volume maps)

// Does not capture mipmap chains, only the top-most texture level is saved

// For 2D array textures and cubemaps, it captures only the first image in the array

#include "ScreenGrab12.h"

#include <assert.h>
#include <algorithm>
#include <memory>

#include <wincodec.h>

#include <wrl\client.h>

#include "d3dx12.h"

using Microsoft::WRL::ComPtr;

//--------------------------------------------------------------------------------------
// Macros
//--------------------------------------------------------------------------------------
#ifndef MAKEFOURCC
#define MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
                ((uint32_t)(uint8_t)(ch0) | ((uint32_t)(uint8_t)(ch1) << 8) |       \
                ((uint32_t)(uint8_t)(ch2) << 16) | ((uint32_t)(uint8_t)(ch3) << 24 ))
#endif /* defined(MAKEFOURCC) */

//--------------------------------------------------------------------------------------
// DDS file structure definitions
//
// See DDS.h in the 'Texconv' sample and the 'DirectXTex' library
//--------------------------------------------------------------------------------------
namespace
{
    #pragma pack(push,1)

    #define DDS_MAGIC 0x20534444 // "DDS "

    struct DDS_PIXELFORMAT
    {
        uint32_t    size;
        uint32_t    flags;
        uint32_t    fourCC;
        uint32_t    RGBBitCount;
        uint32_t    RBitMask;
        uint32_t    GBitMask;
        uint32_t    BBitMask;
        uint32_t    ABitMask;
    };

    #define DDS_FOURCC      0x00000004  // DDPF_FOURCC
    #define DDS_RGB         0x00000040  // DDPF_RGB
    #define DDS_RGBA        0x00000041  // DDPF_RGB | DDPF_ALPHAPIXELS
    #define DDS_LUMINANCE   0x00020000  // DDPF_LUMINANCE
    #define DDS_LUMINANCEA  0x00020001  // DDPF_LUMINANCE | DDPF_ALPHAPIXELS
    #define DDS_ALPHA       0x00000002  // DDPF_ALPHA
    #define DDS_BUMPDUDV    0x00080000  // DDPF_BUMPDUDV

    #define DDS_HEADER_FLAGS_TEXTURE        0x00001007  // DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT 
    #define DDS_HEADER_FLAGS_MIPMAP         0x00020000  // DDSD_MIPMAPCOUNT
    #define DDS_HEADER_FLAGS_PITCH          0x00000008  // DDSD_PITCH
    #define DDS_HEADER_FLAGS_LINEARSIZE     0x00080000  // DDSD_LINEARSIZE

    #define DDS_HEIGHT 0x00000002 // DDSD_HEIGHT
    #define DDS_WIDTH  0x00000004 // DDSD_WIDTH

    #define DDS_SURFACE_FLAGS_TEXTURE 0x00001000 // DDSCAPS_TEXTURE

    typedef struct
    {
        uint32_t        size;
        uint32_t        flags;
        uint32_t        height;
        uint32_t        width;
        uint32_t        pitchOrLinearSize;
        uint32_t        depth; // only if DDS_HEADER_FLAGS_VOLUME is set in flags
        uint32_t        mipMapCount;
        uint32_t        reserved1[11];
        DDS_PIXELFORMAT ddspf;
        uint32_t        caps;
        uint32_t        caps2;
        uint32_t        caps3;
        uint32_t        caps4;
        uint32_t        reserved2;
    } DDS_HEADER;

    typedef struct
    {
        DXGI_FORMAT     dxgiFormat;
        uint32_t        resourceDimension;
        uint32_t        miscFlag; // see D3D11_RESOURCE_MISC_FLAG
        uint32_t        arraySize;
        uint32_t        reserved;
    } DDS_HEADER_DXT10;

    #pragma pack(pop)

    const DDS_PIXELFORMAT DDSPF_DXT1 =
    { sizeof(DDS_PIXELFORMAT), DDS_FOURCC, MAKEFOURCC('D','X','T','1'), 0, 0, 0, 0, 0 };

    const DDS_PIXELFORMAT DDSPF_DXT3 =
    { sizeof(DDS_PIXELFORMAT), DDS_FOURCC, MAKEFOURCC('D','X','T','3'), 0, 0, 0, 0, 0 };

    const DDS_PIXELFORMAT DDSPF_DXT5 =
    { sizeof(DDS_PIXELFORMAT), DDS_FOURCC, MAKEFOURCC('D','X','T','5'), 0, 0, 0, 0, 0 };

    const DDS_PIXELFORMAT DDSPF_BC4_UNORM =
    { sizeof(DDS_PIXELFORMAT), DDS_FOURCC, MAKEFOURCC('B','C','4','U'), 0, 0, 0, 0, 0 };

    const DDS_PIXELFORMAT DDSPF_BC4_SNORM =
    { sizeof(DDS_PIXELFORMAT), DDS_FOURCC, MAKEFOURCC('B','C','4','S'), 0, 0, 0, 0, 0 };

    const DDS_PIXELFORMAT DDSPF_BC5_UNORM =
    { sizeof(DDS_PIXELFORMAT), DDS_FOURCC, MAKEFOURCC('B','C','5','U'), 0, 0, 0, 0, 0 };

    const DDS_PIXELFORMAT DDSPF_BC5_SNORM =
    { sizeof(DDS_PIXELFORMAT), DDS_FOURCC, MAKEFOURCC('B','C','5','S'), 0, 0, 0, 0, 0 };

    const DDS_PIXELFORMAT DDSPF_R8G8_B8G8 =
    { sizeof(DDS_PIXELFORMAT), DDS_FOURCC, MAKEFOURCC('R','G','B','G'), 0, 0, 0, 0, 0 };

    const DDS_PIXELFORMAT DDSPF_G8R8_G8B8 =
    { sizeof(DDS_PIXELFORMAT), DDS_FOURCC, MAKEFOURCC('G','R','G','B'), 0, 0, 0, 0, 0 };

    const DDS_PIXELFORMAT DDSPF_YUY2 =
    { sizeof(DDS_PIXELFORMAT), DDS_FOURCC, MAKEFOURCC('Y','U','Y','2'), 0, 0, 0, 0, 0 };

    const DDS_PIXELFORMAT DDSPF_A8R8G8B8 =
    { sizeof(DDS_PIXELFORMAT), DDS_RGBA, 0, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 };

    const DDS_PIXELFORMAT DDSPF_X8R8G8B8 =
    { sizeof(DDS_PIXELFORMAT), DDS_RGB,  0, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000 };

    const DDS_PIXELFORMAT DDSPF_A8B8G8R8 =
    { sizeof(DDS_PIXELFORMAT), DDS_RGBA, 0, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000 };

    const DDS_PIXELFORMAT DDSPF_G16R16 =
    { sizeof(DDS_PIXELFORMAT), DDS_RGB,  0, 32, 0x0000ffff, 0xffff0000, 0x00000000, 0x00000000 };

    const DDS_PIXELFORMAT DDSPF_R5G6B5 =
    { sizeof(DDS_PIXELFORMAT), DDS_RGB, 0, 16, 0x0000f800, 0x000007e0, 0x0000001f, 0x00000000 };

    const DDS_PIXELFORMAT DDSPF_A1R5G5B5 =
    { sizeof(DDS_PIXELFORMAT), DDS_RGBA, 0, 16, 0x00007c00, 0x000003e0, 0x0000001f, 0x00008000 };

    const DDS_PIXELFORMAT DDSPF_A4R4G4B4 =
    { sizeof(DDS_PIXELFORMAT), DDS_RGBA, 0, 16, 0x00000f00, 0x000000f0, 0x0000000f, 0x0000f000 };

    const DDS_PIXELFORMAT DDSPF_L8 =
    { sizeof(DDS_PIXELFORMAT), DDS_LUMINANCE, 0,  8, 0xff, 0x00, 0x00, 0x00 };

    const DDS_PIXELFORMAT DDSPF_L16 =
    { sizeof(DDS_PIXELFORMAT), DDS_LUMINANCE, 0, 16, 0xffff, 0x0000, 0x0000, 0x0000 };

    const DDS_PIXELFORMAT DDSPF_A8L8 =
    { sizeof(DDS_PIXELFORMAT), DDS_LUMINANCEA, 0, 16, 0x00ff, 0x0000, 0x0000, 0xff00 };

    const DDS_PIXELFORMAT DDSPF_A8 =
    { sizeof(DDS_PIXELFORMAT), DDS_ALPHA, 0, 8, 0x00, 0x00, 0x00, 0xff };

    const DDS_PIXELFORMAT DDSPF_V8U8 =
    { sizeof(DDS_PIXELFORMAT), DDS_BUMPDUDV, 0, 16, 0x00ff, 0xff00, 0x0000, 0x0000 };

    const DDS_PIXELFORMAT DDSPF_Q8W8V8U8 =
    { sizeof(DDS_PIXELFORMAT), DDS_BUMPDUDV, 0, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000 };

    const DDS_PIXELFORMAT DDSPF_V16U16 =
    { sizeof(DDS_PIXELFORMAT), DDS_BUMPDUDV, 0, 32, 0x0000ffff, 0xffff0000, 0x00000000, 0x00000000 };

    // DXGI_FORMAT_R10G10B10A2_UNORM should be written using DX10 extension to avoid D3DX 10:10:10:2 reversal issue

    // This indicates the DDS_HEADER_DXT10 extension is present (the format is in dxgiFormat)
    const DDS_PIXELFORMAT DDSPF_DX10 =
    { sizeof(DDS_PIXELFORMAT), DDS_FOURCC, MAKEFOURCC('D','X','1','0'), 0, 0, 0, 0, 0 };

    //-----------------------------------------------------------------------------
    struct handle_closer { void operator()(HANDLE h) { if (h) CloseHandle(h); } };

    typedef public std::unique_ptr<void, handle_closer> ScopedHandle;

    inline HANDLE safe_handle( HANDLE h ) { return (h == INVALID_HANDLE_VALUE) ? 0 : h; }

    class auto_delete_file
    {
    public:
        auto_delete_file(HANDLE hFile) : m_handle(hFile) {}
        ~auto_delete_file()
        {
            if (m_handle)
            {
                FILE_DISPOSITION_INFO info = {0};
                info.DeleteFile = TRUE;
                (void)SetFileInformationByHandle(m_handle, FileDispositionInfo, &info, sizeof(info));
            }
        }

        void clear() { m_handle = 0; }

    private:
        HANDLE m_handle;

        auto_delete_file(const auto_delete_file&) = delete;
        auto_delete_file& operator=(const auto_delete_file&) = delete;
    };

    class auto_delete_file_wic
    {
    public:
        auto_delete_file_wic(ComPtr<IWICStream>& hFile, LPCWSTR szFile) : m_handle(hFile), m_filename(szFile) {}
        ~auto_delete_file_wic()
        {
            if (m_filename)
            {
                m_handle.Reset();
                DeleteFileW(m_filename);
            }
        }

        void clear() { m_filename = 0; }

    private:
        LPCWSTR m_filename;
        ComPtr<IWICStream>& m_handle;

        auto_delete_file_wic(const auto_delete_file_wic&) = delete;
        auto_delete_file_wic& operator=(const auto_delete_file_wic&) = delete;
    };

    //--------------------------------------------------------------------------------------
    // Return the BPP for a particular format
    //--------------------------------------------------------------------------------------
    size_t BitsPerPixel( _In_ DXGI_FORMAT fmt )
    {
        switch( fmt )
        {
        case DXGI_FORMAT_R32G32B32A32_TYPELESS:
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
        case DXGI_FORMAT_R32G32B32A32_UINT:
        case DXGI_FORMAT_R32G32B32A32_SINT:
            return 128;

        case DXGI_FORMAT_R32G32B32_TYPELESS:
        case DXGI_FORMAT_R32G32B32_FLOAT:
        case DXGI_FORMAT_R32G32B32_UINT:
        case DXGI_FORMAT_R32G32B32_SINT:
            return 96;

        case DXGI_FORMAT_R16G16B16A16_TYPELESS:
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
        case DXGI_FORMAT_R16G16B16A16_UNORM:
        case DXGI_FORMAT_R16G16B16A16_UINT:
        case DXGI_FORMAT_R16G16B16A16_SNORM:
        case DXGI_FORMAT_R16G16B16A16_SINT:
        case DXGI_FORMAT_R32G32_TYPELESS:
        case DXGI_FORMAT_R32G32_FLOAT:
        case DXGI_FORMAT_R32G32_UINT:
        case DXGI_FORMAT_R32G32_SINT:
        case DXGI_FORMAT_R32G8X24_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
        case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
        case DXGI_FORMAT_Y416:
        case DXGI_FORMAT_Y210:
        case DXGI_FORMAT_Y216:
            return 64;

        case DXGI_FORMAT_R10G10B10A2_TYPELESS:
        case DXGI_FORMAT_R10G10B10A2_UNORM:
        case DXGI_FORMAT_R10G10B10A2_UINT:
        case DXGI_FORMAT_R11G11B10_FLOAT:
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        case DXGI_FORMAT_R8G8B8A8_UINT:
        case DXGI_FORMAT_R8G8B8A8_SNORM:
        case DXGI_FORMAT_R8G8B8A8_SINT:
        case DXGI_FORMAT_R16G16_TYPELESS:
        case DXGI_FORMAT_R16G16_FLOAT:
        case DXGI_FORMAT_R16G16_UNORM:
        case DXGI_FORMAT_R16G16_UINT:
        case DXGI_FORMAT_R16G16_SNORM:
        case DXGI_FORMAT_R16G16_SINT:
        case DXGI_FORMAT_R32_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT:
        case DXGI_FORMAT_R32_FLOAT:
        case DXGI_FORMAT_R32_UINT:
        case DXGI_FORMAT_R32_SINT:
        case DXGI_FORMAT_R24G8_TYPELESS:
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
        case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
        case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
        case DXGI_FORMAT_R8G8_B8G8_UNORM:
        case DXGI_FORMAT_G8R8_G8B8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8X8_UNORM:
        case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        case DXGI_FORMAT_B8G8R8X8_TYPELESS:
        case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        case DXGI_FORMAT_AYUV:
        case DXGI_FORMAT_Y410:
        case DXGI_FORMAT_YUY2:
            return 32;

        case DXGI_FORMAT_P010:
        case DXGI_FORMAT_P016:
            return 24;

        case DXGI_FORMAT_R8G8_TYPELESS:
        case DXGI_FORMAT_R8G8_UNORM:
        case DXGI_FORMAT_R8G8_UINT:
        case DXGI_FORMAT_R8G8_SNORM:
        case DXGI_FORMAT_R8G8_SINT:
        case DXGI_FORMAT_R16_TYPELESS:
        case DXGI_FORMAT_R16_FLOAT:
        case DXGI_FORMAT_D16_UNORM:
        case DXGI_FORMAT_R16_UNORM:
        case DXGI_FORMAT_R16_UINT:
        case DXGI_FORMAT_R16_SNORM:
        case DXGI_FORMAT_R16_SINT:
        case DXGI_FORMAT_B5G6R5_UNORM:
        case DXGI_FORMAT_B5G5R5A1_UNORM:
        case DXGI_FORMAT_A8P8:
        case DXGI_FORMAT_B4G4R4A4_UNORM:
            return 16;

        case DXGI_FORMAT_NV12:
        case DXGI_FORMAT_420_OPAQUE:
        case DXGI_FORMAT_NV11:
            return 12;

        case DXGI_FORMAT_R8_TYPELESS:
        case DXGI_FORMAT_R8_UNORM:
        case DXGI_FORMAT_R8_UINT:
        case DXGI_FORMAT_R8_SNORM:
        case DXGI_FORMAT_R8_SINT:
        case DXGI_FORMAT_A8_UNORM:
        case DXGI_FORMAT_AI44:
        case DXGI_FORMAT_IA44:
        case DXGI_FORMAT_P8:
            return 8;

        case DXGI_FORMAT_R1_UNORM:
            return 1;

        case DXGI_FORMAT_BC1_TYPELESS:
        case DXGI_FORMAT_BC1_UNORM:
        case DXGI_FORMAT_BC1_UNORM_SRGB:
        case DXGI_FORMAT_BC4_TYPELESS:
        case DXGI_FORMAT_BC4_UNORM:
        case DXGI_FORMAT_BC4_SNORM:
            return 4;

        case DXGI_FORMAT_BC2_TYPELESS:
        case DXGI_FORMAT_BC2_UNORM:
        case DXGI_FORMAT_BC2_UNORM_SRGB:
        case DXGI_FORMAT_BC3_TYPELESS:
        case DXGI_FORMAT_BC3_UNORM:
        case DXGI_FORMAT_BC3_UNORM_SRGB:
        case DXGI_FORMAT_BC5_TYPELESS:
        case DXGI_FORMAT_BC5_UNORM:
        case DXGI_FORMAT_BC5_SNORM:
        case DXGI_FORMAT_BC6H_TYPELESS:
        case DXGI_FORMAT_BC6H_UF16:
        case DXGI_FORMAT_BC6H_SF16:
        case DXGI_FORMAT_BC7_TYPELESS:
        case DXGI_FORMAT_BC7_UNORM:
        case DXGI_FORMAT_BC7_UNORM_SRGB:
            return 8;

        default:
            return 0;
        }
    }


    //--------------------------------------------------------------------------------------
    // Determines if the format is block compressed
    //--------------------------------------------------------------------------------------
    bool IsCompressed( _In_ DXGI_FORMAT fmt )
    {
        switch ( fmt )
        {
        case DXGI_FORMAT_BC1_TYPELESS:
        case DXGI_FORMAT_BC1_UNORM:
        case DXGI_FORMAT_BC1_UNORM_SRGB:
        case DXGI_FORMAT_BC2_TYPELESS:
        case DXGI_FORMAT_BC2_UNORM:
        case DXGI_FORMAT_BC2_UNORM_SRGB:
        case DXGI_FORMAT_BC3_TYPELESS:
        case DXGI_FORMAT_BC3_UNORM:
        case DXGI_FORMAT_BC3_UNORM_SRGB:
        case DXGI_FORMAT_BC4_TYPELESS:
        case DXGI_FORMAT_BC4_UNORM:
        case DXGI_FORMAT_BC4_SNORM:
        case DXGI_FORMAT_BC5_TYPELESS:
        case DXGI_FORMAT_BC5_UNORM:
        case DXGI_FORMAT_BC5_SNORM:
        case DXGI_FORMAT_BC6H_TYPELESS:
        case DXGI_FORMAT_BC6H_UF16:
        case DXGI_FORMAT_BC6H_SF16:
        case DXGI_FORMAT_BC7_TYPELESS:
        case DXGI_FORMAT_BC7_UNORM:
        case DXGI_FORMAT_BC7_UNORM_SRGB:
            return true;

        default:
            return false;
        }
    }


    //--------------------------------------------------------------------------------------
    // Get surface information for a particular format
    //--------------------------------------------------------------------------------------
    void GetSurfaceInfo(
        _In_ size_t width,
        _In_ size_t height,
        _In_ DXGI_FORMAT fmt,
        _Out_opt_ size_t* outNumBytes,
        _Out_opt_ size_t* outRowBytes,
        _Out_opt_ size_t* outNumRows )
    {
        size_t numBytes = 0;
        size_t rowBytes = 0;
        size_t numRows = 0;

        bool bc = false;
        bool packed = false;
        bool planar = false;
        size_t bpe = 0;
        switch (fmt)
        {
        case DXGI_FORMAT_BC1_TYPELESS:
        case DXGI_FORMAT_BC1_UNORM:
        case DXGI_FORMAT_BC1_UNORM_SRGB:
        case DXGI_FORMAT_BC4_TYPELESS:
        case DXGI_FORMAT_BC4_UNORM:
        case DXGI_FORMAT_BC4_SNORM:
            bc=true;
            bpe = 8;
            break;

        case DXGI_FORMAT_BC2_TYPELESS:
        case DXGI_FORMAT_BC2_UNORM:
        case DXGI_FORMAT_BC2_UNORM_SRGB:
        case DXGI_FORMAT_BC3_TYPELESS:
        case DXGI_FORMAT_BC3_UNORM:
        case DXGI_FORMAT_BC3_UNORM_SRGB:
        case DXGI_FORMAT_BC5_TYPELESS:
        case DXGI_FORMAT_BC5_UNORM:
        case DXGI_FORMAT_BC5_SNORM:
        case DXGI_FORMAT_BC6H_TYPELESS:
        case DXGI_FORMAT_BC6H_UF16:
        case DXGI_FORMAT_BC6H_SF16:
        case DXGI_FORMAT_BC7_TYPELESS:
        case DXGI_FORMAT_BC7_UNORM:
        case DXGI_FORMAT_BC7_UNORM_SRGB:
            bc = true;
            bpe = 16;
            break;

        case DXGI_FORMAT_R8G8_B8G8_UNORM:
        case DXGI_FORMAT_G8R8_G8B8_UNORM:
        case DXGI_FORMAT_YUY2:
            packed = true;
            bpe = 4;
            break;

        case DXGI_FORMAT_Y210:
        case DXGI_FORMAT_Y216:
            packed = true;
            bpe = 8;
            break;

        case DXGI_FORMAT_NV12:
        case DXGI_FORMAT_420_OPAQUE:
            planar = true;
            bpe = 2;
            break;

        case DXGI_FORMAT_P010:
        case DXGI_FORMAT_P016:
            planar = true;
            bpe = 4;
            break;
        }

        if (bc)
        {
            size_t numBlocksWide = 0;
            if (width > 0)
            {
                numBlocksWide = std::max<size_t>( 1, (width + 3) / 4 );
            }
            size_t numBlocksHigh = 0;
            if (height > 0)
            {
                numBlocksHigh = std::max<size_t>( 1, (height + 3) / 4 );
            }
            rowBytes = numBlocksWide * bpe;
            numRows = numBlocksHigh;
            numBytes = rowBytes * numBlocksHigh;
        }
        else if (packed)
        {
            rowBytes = ( ( width + 1 ) >> 1 ) * bpe;
            numRows = height;
            numBytes = rowBytes * height;
        }
        else if ( fmt == DXGI_FORMAT_NV11 )
        {
            rowBytes = ( ( width + 3 ) >> 2 ) * 4;
            numRows = height * 2; // Direct3D makes this simplifying assumption, although it is larger than the 4:1:1 data
            numBytes = rowBytes * numRows;
        }
        else if (planar)
        {
            rowBytes = ( ( width + 1 ) >> 1 ) * bpe;
            numBytes = ( rowBytes * height ) + ( ( rowBytes * height + 1 ) >> 1 );
            numRows = height + ( ( height + 1 ) >> 1 );
        }
        else
        {
            size_t bpp = BitsPerPixel( fmt );
            rowBytes = ( width * bpp + 7 ) / 8; // round up to nearest byte
            numRows = height;
            numBytes = rowBytes * height;
        }

        if (outNumBytes)
        {
            *outNumBytes = numBytes;
        }
        if (outRowBytes)
        {
            *outRowBytes = rowBytes;
        }
        if (outNumRows)
        {
            *outNumRows = numRows;
        }
    }


    //--------------------------------------------------------------------------------------
    DXGI_FORMAT EnsureNotTypeless( DXGI_FORMAT fmt )
    {
        // Assumes UNORM or FLOAT; doesn't use UINT or SINT
        switch( fmt )
        {
        case DXGI_FORMAT_R32G32B32A32_TYPELESS: return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case DXGI_FORMAT_R32G32B32_TYPELESS:    return DXGI_FORMAT_R32G32B32_FLOAT;
        case DXGI_FORMAT_R16G16B16A16_TYPELESS: return DXGI_FORMAT_R16G16B16A16_UNORM;
        case DXGI_FORMAT_R32G32_TYPELESS:       return DXGI_FORMAT_R32G32_FLOAT;
        case DXGI_FORMAT_R10G10B10A2_TYPELESS:  return DXGI_FORMAT_R10G10B10A2_UNORM;
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:     return DXGI_FORMAT_R8G8B8A8_UNORM;
        case DXGI_FORMAT_R16G16_TYPELESS:       return DXGI_FORMAT_R16G16_UNORM;
        case DXGI_FORMAT_R32_TYPELESS:          return DXGI_FORMAT_R32_FLOAT;
        case DXGI_FORMAT_R8G8_TYPELESS:         return DXGI_FORMAT_R8G8_UNORM;
        case DXGI_FORMAT_R16_TYPELESS:          return DXGI_FORMAT_R16_UNORM;
        case DXGI_FORMAT_R8_TYPELESS:           return DXGI_FORMAT_R8_UNORM;
        case DXGI_FORMAT_BC1_TYPELESS:          return DXGI_FORMAT_BC1_UNORM;
        case DXGI_FORMAT_BC2_TYPELESS:          return DXGI_FORMAT_BC2_UNORM;
        case DXGI_FORMAT_BC3_TYPELESS:          return DXGI_FORMAT_BC3_UNORM;
        case DXGI_FORMAT_BC4_TYPELESS:          return DXGI_FORMAT_BC4_UNORM;
        case DXGI_FORMAT_BC5_TYPELESS:          return DXGI_FORMAT_BC5_UNORM;
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:     return DXGI_FORMAT_B8G8R8A8_UNORM;
        case DXGI_FORMAT_B8G8R8X8_TYPELESS:     return DXGI_FORMAT_B8G8R8X8_UNORM;
        case DXGI_FORMAT_BC7_TYPELESS:          return DXGI_FORMAT_BC7_UNORM;
        default:                                return fmt;
        }
    }


    //--------------------------------------------------------------------------------------
    inline void TransitionResource(
        _In_ ID3D12GraphicsCommandList* commandList,
        _In_ ID3D12Resource* resource,
        _In_ D3D12_RESOURCE_STATES stateBefore,
        _In_ D3D12_RESOURCE_STATES stateAfter)
    {
        assert(commandList != 0);
        assert(resource != 0);

        if (stateBefore == stateAfter)
            return;

        D3D12_RESOURCE_BARRIER desc = {};
        desc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        desc.Transition.pResource = resource;
        desc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        desc.Transition.StateBefore = stateBefore;
        desc.Transition.StateAfter = stateAfter;

        commandList->ResourceBarrier(1, &desc);
    }


    //--------------------------------------------------------------------------------------
    HRESULT CaptureTexture(_In_ ID3D12Device* device,
        _In_ ID3D12CommandQueue* pCommandQ,
        _In_ ID3D12Resource* pSource,
        UINT64 srcPitch,
        const D3D12_RESOURCE_DESC& desc,
        ComPtr<ID3D12Resource>& pStaging,
        D3D12_RESOURCE_STATES beforeState,
        D3D12_RESOURCE_STATES afterState)
    {
        if (!pCommandQ || !pSource)
            return E_INVALIDARG;

        if (desc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D)
            return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);

        UINT numberOfPlanes = D3D12GetFormatPlaneCount(device, desc.Format);
        if (numberOfPlanes != 1)
            return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);

        D3D12_HEAP_PROPERTIES sourceHeapProperties;
        D3D12_HEAP_FLAGS sourceHeapFlags;
        HRESULT hr = pSource->GetHeapProperties(&sourceHeapProperties, &sourceHeapFlags);
        if (FAILED(hr))
            return hr;

        if (sourceHeapProperties.Type == D3D12_HEAP_TYPE_READBACK)
        {
            // Handle case where the source is already a staging texture we can use directly
            pStaging = pSource;
            return S_OK;
        }

        // Create a command allocator
        ComPtr<ID3D12CommandAllocator> commandAlloc;
        hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(commandAlloc.GetAddressOf()));
        if (FAILED(hr))
            return hr;

        // Spin up a new command list
        ComPtr<ID3D12GraphicsCommandList> commandList;
        hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAlloc.Get(), nullptr, IID_PPV_ARGS(commandList.GetAddressOf()));
        if (FAILED(hr))
            return hr;

        // Create a fence
        ComPtr<ID3D12Fence> fence;
        hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence.GetAddressOf()));
        if (FAILED(hr))
            return hr;

        assert((srcPitch & 0xFF) == 0);

        CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);
        CD3DX12_HEAP_PROPERTIES readBackHeapProperties(D3D12_HEAP_TYPE_READBACK);

        // Readback resources must be buffers
        D3D12_RESOURCE_DESC bufferDesc = {};
        bufferDesc.Alignment = desc.Alignment;
        bufferDesc.DepthOrArraySize = 1;
        bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
        bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
        bufferDesc.Height = 1;
        bufferDesc.Width = srcPitch * desc.Height;
        bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        bufferDesc.MipLevels = 1;
        bufferDesc.SampleDesc.Count = 1;
        bufferDesc.SampleDesc.Quality = 0;

        ComPtr<ID3D12Resource> copySource(pSource);
        if (desc.SampleDesc.Count > 1)
        {
            // MSAA content must be resolved before being copied to a staging texture
            auto descCopy = desc;
            descCopy.SampleDesc.Count = 1;
            descCopy.SampleDesc.Quality = 0;

            ComPtr<ID3D12Resource> pTemp;
            hr = device->CreateCommittedResource(
                &defaultHeapProperties,
                D3D12_HEAP_FLAG_NONE,
                &descCopy,
                D3D12_RESOURCE_STATE_COPY_DEST,
                nullptr,
                IID_PPV_ARGS(pTemp.GetAddressOf()));
            if (FAILED(hr))
                return hr;

            assert(pTemp);

            DXGI_FORMAT fmt = EnsureNotTypeless(desc.Format);

            D3D12_FEATURE_DATA_FORMAT_SUPPORT formatInfo = { fmt };
            hr = device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &formatInfo, sizeof(formatInfo));
            if (FAILED(hr))
                return hr;

            if (!(formatInfo.Support1 & D3D12_FORMAT_SUPPORT1_TEXTURE2D))
                return E_FAIL;

            for (UINT item = 0; item < desc.DepthOrArraySize; ++item)
            {
                for (UINT level = 0; level < desc.MipLevels; ++level)
                {
                    UINT index = D3D12CalcSubresource(level, item, 0, desc.MipLevels, desc.DepthOrArraySize);
                    commandList->ResolveSubresource(pTemp.Get(), index, pSource, index, fmt);
                }
            }

            copySource = pTemp;
        }

        // Create a staging texture
        hr = device->CreateCommittedResource(
            &readBackHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &bufferDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(pStaging.ReleaseAndGetAddressOf()));
        if (FAILED(hr))
            return hr;

        assert(pStaging);

        // Transition the resource if necessary
        TransitionResource(commandList.Get(), pSource, beforeState, D3D12_RESOURCE_STATE_COPY_SOURCE);

        // Get the copy target location
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT bufferFootprint = {};
        bufferFootprint.Footprint.Width = static_cast<UINT>(desc.Width);
        bufferFootprint.Footprint.Height = desc.Height;
        bufferFootprint.Footprint.Depth = 1;
        bufferFootprint.Footprint.RowPitch = static_cast<UINT>(srcPitch);
        bufferFootprint.Footprint.Format = desc.Format;

        CD3DX12_TEXTURE_COPY_LOCATION copyDest(pStaging.Get(), bufferFootprint);
        CD3DX12_TEXTURE_COPY_LOCATION copySrc(copySource.Get(), 0);

        // Copy the texture
        commandList->CopyTextureRegion(&copyDest, 0, 0, 0, &copySrc, nullptr);

        // Transition the resource to the next state
        TransitionResource(commandList.Get(), pSource, D3D12_RESOURCE_STATE_COPY_SOURCE, afterState);

        hr = commandList->Close();
        if (FAILED(hr))
            return hr;

        // Execute the command list
        pCommandQ->ExecuteCommandLists(1, (ID3D12CommandList**)commandList.GetAddressOf());

        // Signal the fence
        hr = pCommandQ->Signal(fence.Get(), 1);
        if (FAILED(hr))
            return hr;

        // Block until the copy is complete
        while (fence->GetCompletedValue() < 1)
            SwitchToThread();

        return S_OK;
    }

    IWICImagingFactory2* _GetWIC()
    {
        static INIT_ONCE s_initOnce = INIT_ONCE_STATIC_INIT;

        IWICImagingFactory2* factory = nullptr;
        (void)InitOnceExecuteOnce(&s_initOnce,
            [](PINIT_ONCE, PVOID, PVOID *factory) -> BOOL
            {
                return SUCCEEDED( CoCreateInstance(
                    CLSID_WICImagingFactory2,
                    nullptr,
                    CLSCTX_INPROC_SERVER,
                    __uuidof(IWICImagingFactory2),
                    factory) ) ? TRUE : FALSE;
            }, nullptr, reinterpret_cast<LPVOID*>(&factory));

        return factory;
    }
} // anonymous namespace


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT DirectX::SaveDDSTextureToFile( ID3D12CommandQueue* pCommandQ,
                                       ID3D12Resource* pSource,
                                       const wchar_t* fileName,
                                       D3D12_RESOURCE_STATES beforeState,
                                       D3D12_RESOURCE_STATES afterState)
{
    if ( !fileName )
        return E_INVALIDARG;

    ComPtr<ID3D12Device> device;
    pCommandQ->GetDevice(IID_PPV_ARGS(device.GetAddressOf()));

    // Get the size of the image
    D3D12_RESOURCE_DESC desc = pSource->GetDesc();
    UINT64 totalResourceSize = 0;
    UINT64 fpRowPitch = 0;
    UINT fpRowCount = 0;
    // Get the rowcount, pitch and size of the top mip
    device->GetCopyableFootprints(
        &desc,
        0,
        1,
        0,
        nullptr,
        &fpRowCount,
        &fpRowPitch,
        &totalResourceSize);

    // Round up the srcPitch to multiples of 256
    UINT64 dstRowPitch = (fpRowPitch + 255) & ~0xFF;

    ComPtr<ID3D12Resource> pStaging;
    HRESULT hr = CaptureTexture( device.Get(), pCommandQ, pSource, dstRowPitch, desc, pStaging, beforeState, afterState );
    if ( FAILED(hr) )
        return hr;

    // Create file
    ScopedHandle hFile( safe_handle( CreateFile2( fileName, GENERIC_WRITE, 0, CREATE_ALWAYS, nullptr ) ) );
    if ( !hFile )
        return HRESULT_FROM_WIN32( GetLastError() );

    auto_delete_file delonfail(hFile.get());

    // Setup header
    const size_t MAX_HEADER_SIZE = sizeof(uint32_t) + sizeof(DDS_HEADER) + sizeof(DDS_HEADER_DXT10);
    uint8_t fileHeader[ MAX_HEADER_SIZE ];

    *reinterpret_cast<uint32_t*>(&fileHeader[0]) = DDS_MAGIC;

    auto header = reinterpret_cast<DDS_HEADER*>( &fileHeader[0] + sizeof(uint32_t) );
    size_t headerSize = sizeof(uint32_t) + sizeof(DDS_HEADER);
    memset( header, 0, sizeof(DDS_HEADER) );
    header->size = sizeof( DDS_HEADER );
    header->flags = DDS_HEADER_FLAGS_TEXTURE | DDS_HEADER_FLAGS_MIPMAP;
    header->height = desc.Height;
    header->width = (uint32_t) desc.Width;
    header->mipMapCount = 1;
    header->caps = DDS_SURFACE_FLAGS_TEXTURE;

    // Try to use a legacy .DDS pixel format for better tools support, otherwise fallback to 'DX10' header extension
    DDS_HEADER_DXT10* extHeader = nullptr;
    switch( desc.Format )
    {
    case DXGI_FORMAT_R8G8B8A8_UNORM:        memcpy_s( &header->ddspf, sizeof(header->ddspf), &DDSPF_A8B8G8R8, sizeof(DDS_PIXELFORMAT) );    break;
    case DXGI_FORMAT_R16G16_UNORM:          memcpy_s( &header->ddspf, sizeof(header->ddspf), &DDSPF_G16R16, sizeof(DDS_PIXELFORMAT) );      break;
    case DXGI_FORMAT_R8G8_UNORM:            memcpy_s( &header->ddspf, sizeof(header->ddspf), &DDSPF_A8L8, sizeof(DDS_PIXELFORMAT) );        break;
    case DXGI_FORMAT_R16_UNORM:             memcpy_s( &header->ddspf, sizeof(header->ddspf), &DDSPF_L16, sizeof(DDS_PIXELFORMAT) );         break;
    case DXGI_FORMAT_R8_UNORM:              memcpy_s( &header->ddspf, sizeof(header->ddspf), &DDSPF_L8, sizeof(DDS_PIXELFORMAT) );          break;
    case DXGI_FORMAT_A8_UNORM:              memcpy_s( &header->ddspf, sizeof(header->ddspf), &DDSPF_A8, sizeof(DDS_PIXELFORMAT) );          break;
    case DXGI_FORMAT_R8G8_B8G8_UNORM:       memcpy_s( &header->ddspf, sizeof(header->ddspf), &DDSPF_R8G8_B8G8, sizeof(DDS_PIXELFORMAT) );   break;
    case DXGI_FORMAT_G8R8_G8B8_UNORM:       memcpy_s( &header->ddspf, sizeof(header->ddspf), &DDSPF_G8R8_G8B8, sizeof(DDS_PIXELFORMAT) );   break;
    case DXGI_FORMAT_BC1_UNORM:             memcpy_s( &header->ddspf, sizeof(header->ddspf), &DDSPF_DXT1, sizeof(DDS_PIXELFORMAT) );        break;
    case DXGI_FORMAT_BC2_UNORM:             memcpy_s( &header->ddspf, sizeof(header->ddspf), &DDSPF_DXT3, sizeof(DDS_PIXELFORMAT) );        break;
    case DXGI_FORMAT_BC3_UNORM:             memcpy_s( &header->ddspf, sizeof(header->ddspf), &DDSPF_DXT5, sizeof(DDS_PIXELFORMAT) );        break;
    case DXGI_FORMAT_BC4_UNORM:             memcpy_s( &header->ddspf, sizeof(header->ddspf), &DDSPF_BC4_UNORM, sizeof(DDS_PIXELFORMAT) );   break;
    case DXGI_FORMAT_BC4_SNORM:             memcpy_s( &header->ddspf, sizeof(header->ddspf), &DDSPF_BC4_SNORM, sizeof(DDS_PIXELFORMAT) );   break;
    case DXGI_FORMAT_BC5_UNORM:             memcpy_s( &header->ddspf, sizeof(header->ddspf), &DDSPF_BC5_UNORM, sizeof(DDS_PIXELFORMAT) );   break;
    case DXGI_FORMAT_BC5_SNORM:             memcpy_s( &header->ddspf, sizeof(header->ddspf), &DDSPF_BC5_SNORM, sizeof(DDS_PIXELFORMAT) );   break;
    case DXGI_FORMAT_B5G6R5_UNORM:          memcpy_s( &header->ddspf, sizeof(header->ddspf), &DDSPF_R5G6B5, sizeof(DDS_PIXELFORMAT) );      break;
    case DXGI_FORMAT_B5G5R5A1_UNORM:        memcpy_s( &header->ddspf, sizeof(header->ddspf), &DDSPF_A1R5G5B5, sizeof(DDS_PIXELFORMAT) );    break;
    case DXGI_FORMAT_R8G8_SNORM:            memcpy_s( &header->ddspf, sizeof(header->ddspf), &DDSPF_V8U8, sizeof(DDS_PIXELFORMAT) );        break;
    case DXGI_FORMAT_R8G8B8A8_SNORM:        memcpy_s( &header->ddspf, sizeof(header->ddspf), &DDSPF_Q8W8V8U8, sizeof(DDS_PIXELFORMAT) );    break;
    case DXGI_FORMAT_R16G16_SNORM:          memcpy_s( &header->ddspf, sizeof(header->ddspf), &DDSPF_V16U16, sizeof(DDS_PIXELFORMAT) );      break;
    case DXGI_FORMAT_B8G8R8A8_UNORM:        memcpy_s( &header->ddspf, sizeof(header->ddspf), &DDSPF_A8R8G8B8, sizeof(DDS_PIXELFORMAT) );    break;
    case DXGI_FORMAT_B8G8R8X8_UNORM:        memcpy_s( &header->ddspf, sizeof(header->ddspf), &DDSPF_X8R8G8B8, sizeof(DDS_PIXELFORMAT) );    break;
    case DXGI_FORMAT_YUY2:                  memcpy_s( &header->ddspf, sizeof(header->ddspf), &DDSPF_YUY2, sizeof(DDS_PIXELFORMAT) );        break;
    case DXGI_FORMAT_B4G4R4A4_UNORM:        memcpy_s( &header->ddspf, sizeof(header->ddspf), &DDSPF_A4R4G4B4, sizeof(DDS_PIXELFORMAT) );    break;

    // Legacy D3DX formats using D3DFMT enum value as FourCC
    case DXGI_FORMAT_R32G32B32A32_FLOAT:    header->ddspf.size = sizeof(DDS_PIXELFORMAT); header->ddspf.flags = DDS_FOURCC; header->ddspf.fourCC = 116; break; // D3DFMT_A32B32G32R32F
    case DXGI_FORMAT_R16G16B16A16_FLOAT:    header->ddspf.size = sizeof(DDS_PIXELFORMAT); header->ddspf.flags = DDS_FOURCC; header->ddspf.fourCC = 113; break; // D3DFMT_A16B16G16R16F
    case DXGI_FORMAT_R16G16B16A16_UNORM:    header->ddspf.size = sizeof(DDS_PIXELFORMAT); header->ddspf.flags = DDS_FOURCC; header->ddspf.fourCC = 36;  break; // D3DFMT_A16B16G16R16
    case DXGI_FORMAT_R16G16B16A16_SNORM:    header->ddspf.size = sizeof(DDS_PIXELFORMAT); header->ddspf.flags = DDS_FOURCC; header->ddspf.fourCC = 110; break; // D3DFMT_Q16W16V16U16
    case DXGI_FORMAT_R32G32_FLOAT:          header->ddspf.size = sizeof(DDS_PIXELFORMAT); header->ddspf.flags = DDS_FOURCC; header->ddspf.fourCC = 115; break; // D3DFMT_G32R32F
    case DXGI_FORMAT_R16G16_FLOAT:          header->ddspf.size = sizeof(DDS_PIXELFORMAT); header->ddspf.flags = DDS_FOURCC; header->ddspf.fourCC = 112; break; // D3DFMT_G16R16F
    case DXGI_FORMAT_R32_FLOAT:             header->ddspf.size = sizeof(DDS_PIXELFORMAT); header->ddspf.flags = DDS_FOURCC; header->ddspf.fourCC = 114; break; // D3DFMT_R32F
    case DXGI_FORMAT_R16_FLOAT:             header->ddspf.size = sizeof(DDS_PIXELFORMAT); header->ddspf.flags = DDS_FOURCC; header->ddspf.fourCC = 111; break; // D3DFMT_R16F

    case DXGI_FORMAT_AI44:
    case DXGI_FORMAT_IA44:
    case DXGI_FORMAT_P8:
    case DXGI_FORMAT_A8P8:
        return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );

    default:
        memcpy_s( &header->ddspf, sizeof(header->ddspf), &DDSPF_DX10, sizeof(DDS_PIXELFORMAT) );

        headerSize += sizeof(DDS_HEADER_DXT10);
        extHeader = reinterpret_cast<DDS_HEADER_DXT10*>( reinterpret_cast<uint8_t*>(&fileHeader[0]) + sizeof(uint32_t) + sizeof(DDS_HEADER) );
        memset( extHeader, 0, sizeof(DDS_HEADER_DXT10) );
        extHeader->dxgiFormat = desc.Format;
        extHeader->resourceDimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        extHeader->arraySize = 1;
        break;
    }

    size_t rowPitch, slicePitch, rowCount;
    GetSurfaceInfo( (size_t)desc.Width, desc.Height, desc.Format, &slicePitch, &rowPitch, &rowCount );

    if ( IsCompressed( desc.Format ) )
    {
        header->flags |= DDS_HEADER_FLAGS_LINEARSIZE;
        header->pitchOrLinearSize = static_cast<uint32_t>( slicePitch );
    }
    else
    {
        header->flags |= DDS_HEADER_FLAGS_PITCH;
        header->pitchOrLinearSize = static_cast<uint32_t>( rowPitch );
    }

    // Setup pixels
    std::unique_ptr<uint8_t[]> pixels( new (std::nothrow) uint8_t[ slicePitch ] );
    if (!pixels)
        return E_OUTOFMEMORY;

    assert(fpRowCount == rowCount);
    assert(fpRowPitch == rowPitch);

    void* pMappedMemory;
    D3D12_RANGE readRange = { 0, static_cast<SIZE_T>(dstRowPitch * rowCount) };
    D3D12_RANGE writeRange = { 0, 0 };
    hr = pStaging->Map(0, &readRange, &pMappedMemory );
    if ( FAILED(hr) )
        return hr;

    auto sptr = reinterpret_cast<const uint8_t*>(pMappedMemory);
    if ( !sptr )
    {
        pStaging->Unmap(0, &writeRange);
        return E_POINTER;
    }

    uint8_t* dptr = pixels.get();

    size_t msize = std::min<size_t>( rowPitch, rowPitch );
    for( size_t h = 0; h < rowCount; ++h )
    {
        memcpy_s( dptr, rowPitch, sptr, msize );
        sptr += dstRowPitch;
        dptr += rowPitch;
    }

    pStaging->Unmap( 0, &writeRange );

    // Write header & pixels
    DWORD bytesWritten;
    if ( !WriteFile( hFile.get(), fileHeader, static_cast<DWORD>( headerSize ), &bytesWritten, nullptr ) )
        return HRESULT_FROM_WIN32( GetLastError() );

    if ( bytesWritten != headerSize )
        return E_FAIL;

    if ( !WriteFile( hFile.get(), pixels.get(), static_cast<DWORD>( slicePitch ), &bytesWritten, nullptr ) )
        return HRESULT_FROM_WIN32( GetLastError() );

    if ( bytesWritten != slicePitch )
        return E_FAIL;

    delonfail.clear();

    return S_OK;
}

//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT DirectX::SaveWICTextureToFile( ID3D12CommandQueue* pCommandQ,
                                       ID3D12Resource* pSource,
                                       REFGUID guidContainerFormat, 
                                       const wchar_t* fileName,
                                       D3D12_RESOURCE_STATES beforeState,
                                       D3D12_RESOURCE_STATES afterState,
                                       const GUID* targetFormat,
                                       std::function<void(IPropertyBag2*)> setCustomProps )
{
    if ( !fileName )
        return E_INVALIDARG;

    ComPtr<ID3D12Device> device;
    pCommandQ->GetDevice(IID_PPV_ARGS(device.GetAddressOf()));

    // Get the size of the image
    D3D12_RESOURCE_DESC desc = pSource->GetDesc();
    UINT64 totalResourceSize = 0;
    UINT64 fpRowPitch = 0;
    UINT fpRowCount = 0;
    // Get the rowcount, pitch and size of the top mip
    device->GetCopyableFootprints(
        &desc,
        0,
        1,
        0,
        nullptr,
        &fpRowCount,
        &fpRowPitch,
        &totalResourceSize);

    // Round up the srcPitch to multiples of 256
    UINT64 dstRowPitch = (fpRowPitch + 255) & ~0xFF;

    ComPtr<ID3D12Resource> pStaging;
    HRESULT hr = CaptureTexture(device.Get(), pCommandQ, pSource, dstRowPitch, desc, pStaging, beforeState, afterState);
    if (FAILED(hr))
        return hr;

    // Determine source format's WIC equivalent
    WICPixelFormatGUID pfGuid;
    bool sRGB = false;
    switch ( desc.Format )
    {
    case DXGI_FORMAT_R32G32B32A32_FLOAT:            pfGuid = GUID_WICPixelFormat128bppRGBAFloat; break;
    case DXGI_FORMAT_R16G16B16A16_FLOAT:            pfGuid = GUID_WICPixelFormat64bppRGBAHalf; break;
    case DXGI_FORMAT_R16G16B16A16_UNORM:            pfGuid = GUID_WICPixelFormat64bppRGBA; break;
    case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:    pfGuid = GUID_WICPixelFormat32bppRGBA1010102XR; break;
    case DXGI_FORMAT_R10G10B10A2_UNORM:             pfGuid = GUID_WICPixelFormat32bppRGBA1010102; break;
    case DXGI_FORMAT_B5G5R5A1_UNORM:                pfGuid = GUID_WICPixelFormat16bppBGRA5551; break;
    case DXGI_FORMAT_B5G6R5_UNORM:                  pfGuid = GUID_WICPixelFormat16bppBGR565; break;
    case DXGI_FORMAT_R32_FLOAT:                     pfGuid = GUID_WICPixelFormat32bppGrayFloat; break;
    case DXGI_FORMAT_R16_FLOAT:                     pfGuid = GUID_WICPixelFormat16bppGrayHalf; break;
    case DXGI_FORMAT_R16_UNORM:                     pfGuid = GUID_WICPixelFormat16bppGray; break;
    case DXGI_FORMAT_R8_UNORM:                      pfGuid = GUID_WICPixelFormat8bppGray; break;
    case DXGI_FORMAT_A8_UNORM:                      pfGuid = GUID_WICPixelFormat8bppAlpha; break;

    case DXGI_FORMAT_R8G8B8A8_UNORM:
        pfGuid = GUID_WICPixelFormat32bppRGBA;
        break;

    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        pfGuid = GUID_WICPixelFormat32bppRGBA;
        sRGB = true;
        break;

    case DXGI_FORMAT_B8G8R8A8_UNORM:
        pfGuid = GUID_WICPixelFormat32bppBGRA;
        break;

    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        pfGuid = GUID_WICPixelFormat32bppBGRA;
        sRGB = true;
        break;

    case DXGI_FORMAT_B8G8R8X8_UNORM:
        pfGuid = GUID_WICPixelFormat32bppBGR;
        break; 

    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        pfGuid = GUID_WICPixelFormat32bppBGR;
        sRGB = true;
        break; 

    default:
        return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
    }

    auto pWIC = _GetWIC();
    if ( !pWIC )
        return E_NOINTERFACE;

    ComPtr<IWICStream> stream;
    hr = pWIC->CreateStream( stream.GetAddressOf() );
    if ( FAILED(hr) )
        return hr;

    hr = stream->InitializeFromFilename( fileName, GENERIC_WRITE );
    if ( FAILED(hr) )
        return hr;

    auto_delete_file_wic delonfail(stream, fileName);

    ComPtr<IWICBitmapEncoder> encoder;
    hr = pWIC->CreateEncoder( guidContainerFormat, 0, encoder.GetAddressOf() );
    if ( FAILED(hr) )
        return hr;

    hr = encoder->Initialize( stream.Get(), WICBitmapEncoderNoCache );
    if ( FAILED(hr) )
        return hr;

    ComPtr<IWICBitmapFrameEncode> frame;
    ComPtr<IPropertyBag2> props;
    hr = encoder->CreateNewFrame( frame.GetAddressOf(), props.GetAddressOf() );
    if ( FAILED(hr) )
        return hr;

    if ( targetFormat && memcmp( &guidContainerFormat, &GUID_ContainerFormatBmp, sizeof(WICPixelFormatGUID) ) == 0 )
    {
        // Opt-in to the WIC2 support for writing 32-bit Windows BMP files with an alpha channel
        PROPBAG2 option = {};
        option.pstrName = const_cast<wchar_t*>(L"EnableV5Header32bppBGRA");

        VARIANT varValue;    
        varValue.vt = VT_BOOL;
        varValue.boolVal = VARIANT_TRUE;      
        (void)props->Write( 1, &option, &varValue ); 
    }

    if ( setCustomProps )
    {
        setCustomProps( props.Get() );
    }

    hr = frame->Initialize( props.Get() );
    if ( FAILED(hr) )
        return hr;

    hr = frame->SetSize(static_cast<UINT>(desc.Width), desc.Height );
    if ( FAILED(hr) )
        return hr;

    hr = frame->SetResolution( 72, 72 );
    if ( FAILED(hr) )
        return hr;

    // Pick a target format
    WICPixelFormatGUID targetGuid;
    if ( targetFormat )
    {
        targetGuid = *targetFormat;
    }
    else
    {
        // Screenshots don’t typically include the alpha channel of the render target
        switch ( desc.Format )
        {
        case DXGI_FORMAT_R32G32B32A32_FLOAT:            
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
            targetGuid = GUID_WICPixelFormat96bppRGBFloat; // WIC 2
            break;

        case DXGI_FORMAT_R16G16B16A16_UNORM: targetGuid = GUID_WICPixelFormat48bppBGR; break;
        case DXGI_FORMAT_B5G5R5A1_UNORM:     targetGuid = GUID_WICPixelFormat16bppBGR555; break;
        case DXGI_FORMAT_B5G6R5_UNORM:       targetGuid = GUID_WICPixelFormat16bppBGR565; break;

        case DXGI_FORMAT_R32_FLOAT:
        case DXGI_FORMAT_R16_FLOAT:
        case DXGI_FORMAT_R16_UNORM:
        case DXGI_FORMAT_R8_UNORM:
        case DXGI_FORMAT_A8_UNORM:
            targetGuid = GUID_WICPixelFormat8bppGray;
            break;

        default:
            targetGuid = GUID_WICPixelFormat24bppBGR;
            break;
        }
    }

    hr = frame->SetPixelFormat( &targetGuid );
    if ( FAILED(hr) )
        return hr;

    if ( targetFormat && memcmp( targetFormat, &targetGuid, sizeof(WICPixelFormatGUID) ) != 0 )
    {
        // Requested output pixel format is not supported by the WIC codec
        return E_FAIL;
    }

    // Encode WIC metadata
    ComPtr<IWICMetadataQueryWriter> metawriter;
    if ( SUCCEEDED( frame->GetMetadataQueryWriter( metawriter.GetAddressOf() ) ) )
    {
        PROPVARIANT value;
        PropVariantInit( &value );

        value.vt = VT_LPSTR;
        value.pszVal = const_cast<char*>("DirectXTK");

        if ( memcmp( &guidContainerFormat, &GUID_ContainerFormatPng, sizeof(GUID) ) == 0 )
        {
            // Set Software name
            (void)metawriter->SetMetadataByName( L"/tEXt/{str=Software}", &value );

            // Set sRGB chunk
            if (sRGB)
            {
                value.vt = VT_UI1;
                value.bVal = 0;
                (void)metawriter->SetMetadataByName(L"/sRGB/RenderingIntent", &value);
            }
            else
            {
                // add gAMA chunk with gamma 1.0
                value.vt = VT_UI4;
                value.uintVal = 100000; // gama value * 100,000 -- i.e. gamma 1.0
                (void)metawriter->SetMetadataByName(L"/gAMA/ImageGamma", &value);

                // remove sRGB chunk which is added by default.
                (void)metawriter->RemoveMetadataByName(L"/sRGB/RenderingIntent");
            }
        }
        else
        {
            // Set Software name
            (void)metawriter->SetMetadataByName( L"System.ApplicationName", &value );

            if ( sRGB )
            {
                // Set EXIF Colorspace of sRGB
                value.vt = VT_UI2;
                value.uiVal = 1;
                (void)metawriter->SetMetadataByName( L"System.Image.ColorSpace", &value );
            }
        }
    }

    void* pMappedMemory;
    D3D12_RANGE readRange = {0, static_cast<SIZE_T>(dstRowPitch * desc.Height)};
    D3D12_RANGE writeRange = {0, 0};
    hr = pStaging->Map(0, &readRange, &pMappedMemory);
    if (FAILED(hr))
        return hr;

    if ( memcmp( &targetGuid, &pfGuid, sizeof(WICPixelFormatGUID) ) != 0 )
    {
        // Conversion required to write
        ComPtr<IWICBitmap> source;
        hr = pWIC->CreateBitmapFromMemory(static_cast<UINT>(desc.Width), desc.Height, pfGuid,
                                          static_cast<UINT>(dstRowPitch), static_cast<UINT>(dstRowPitch * desc.Height),
                                           reinterpret_cast<BYTE*>(pMappedMemory), source.GetAddressOf() );
        if ( FAILED(hr) )
        {
            pStaging->Unmap( 0, &writeRange );
            return hr;
        }

        ComPtr<IWICFormatConverter> FC;
        hr = pWIC->CreateFormatConverter( FC.GetAddressOf() );
        if ( FAILED(hr) )
        {
            pStaging->Unmap(0, &writeRange);
            return hr;
        }

        BOOL canConvert = FALSE;
        hr = FC->CanConvert( pfGuid, targetGuid, &canConvert );
        if ( FAILED(hr) || !canConvert )
        {
            return E_UNEXPECTED;
        }

        hr = FC->Initialize( source.Get(), targetGuid, WICBitmapDitherTypeNone, nullptr, 0, WICBitmapPaletteTypeMedianCut );
        if ( FAILED(hr) )
        {
            pStaging->Unmap(0, &writeRange);
            return hr;
        }

        WICRect rect = { 0, 0, static_cast<INT>( desc.Width ), static_cast<INT>( desc.Height ) };
        hr = frame->WriteSource( FC.Get(), &rect );
        if ( FAILED(hr) )
        {
            pStaging->Unmap(0, &writeRange);
            return hr;
        }
    }
    else
    {
        // No conversion required
        hr = frame->WritePixels( desc.Height, static_cast<UINT>(dstRowPitch), static_cast<UINT>(dstRowPitch * desc.Height), reinterpret_cast<BYTE*>( pMappedMemory ) );
        if ( FAILED(hr) )
            return hr;
    }

    pStaging->Unmap(0, &writeRange);

    hr = frame->Commit();
    if ( FAILED(hr) )
        return hr;

    hr = encoder->Commit();
    if ( FAILED(hr) )
        return hr;

    delonfail.clear();

    return S_OK;
}
