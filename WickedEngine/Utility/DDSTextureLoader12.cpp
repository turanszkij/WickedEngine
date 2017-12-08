//--------------------------------------------------------------------------------------
// File: DDSTextureLoader12.cpp
//
// Functions for loading a DDS texture and creating a Direct3D runtime resource for it
//
// Note these functions are useful as a light-weight runtime loader for DDS files. For
// a full-featured DDS file reader, writer, and texture processing pipeline see
// the 'Texconv' sample and the 'DirectXTex' library.
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

#include "DDSTextureLoader12.h"

#include <assert.h>
#include <algorithm>
#include <memory>

#include "d3dx12.h"

using namespace DirectX;

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
#pragma pack(push,1)

const uint32_t DDS_MAGIC = 0x20534444; // "DDS "

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
#define DDS_LUMINANCE   0x00020000  // DDPF_LUMINANCE
#define DDS_ALPHA       0x00000002  // DDPF_ALPHA
#define DDS_BUMPDUDV    0x00080000  // DDPF_BUMPDUDV

#define DDS_HEADER_FLAGS_VOLUME         0x00800000  // DDSD_DEPTH

#define DDS_HEIGHT 0x00000002 // DDSD_HEIGHT
#define DDS_WIDTH  0x00000004 // DDSD_WIDTH

#define DDS_CUBEMAP_POSITIVEX 0x00000600 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEX
#define DDS_CUBEMAP_NEGATIVEX 0x00000a00 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEX
#define DDS_CUBEMAP_POSITIVEY 0x00001200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEY
#define DDS_CUBEMAP_NEGATIVEY 0x00002200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEY
#define DDS_CUBEMAP_POSITIVEZ 0x00004200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEZ
#define DDS_CUBEMAP_NEGATIVEZ 0x00008200 // DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEZ

#define DDS_CUBEMAP_ALLFACES ( DDS_CUBEMAP_POSITIVEX | DDS_CUBEMAP_NEGATIVEX |\
                               DDS_CUBEMAP_POSITIVEY | DDS_CUBEMAP_NEGATIVEY |\
                               DDS_CUBEMAP_POSITIVEZ | DDS_CUBEMAP_NEGATIVEZ )

#define DDS_CUBEMAP 0x00000200 // DDSCAPS2_CUBEMAP

enum DDS_MISC_FLAGS2
{
    DDS_MISC_FLAGS2_ALPHA_MODE_MASK = 0x7L,
};

struct DDS_HEADER
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
};

struct DDS_HEADER_DXT10
{
    DXGI_FORMAT     dxgiFormat;
    uint32_t        resourceDimension;
    uint32_t        miscFlag; // see D3D11_RESOURCE_MISC_FLAG
    uint32_t        arraySize;
    uint32_t        miscFlags2;
};

#pragma pack(pop)

//--------------------------------------------------------------------------------------
namespace
{
    struct handle_closer { void operator()(HANDLE h) { if (h) CloseHandle(h); } };

    typedef public std::unique_ptr<void, handle_closer> ScopedHandle;

    inline HANDLE safe_handle(HANDLE h) { return (h == INVALID_HANDLE_VALUE) ? 0 : h; }

    template<UINT TNameLength>
    inline void SetDebugObjectName(_In_ ID3D12DeviceChild* resource, _In_z_ const wchar_t(&name)[TNameLength])
    {
        #if !defined(NO_D3D12_DEBUG_NAME) && ( defined(_DEBUG) || defined(PROFILE) )
            resource->SetName(name);
        #else
            UNREFERENCED_PARAMETER(resource);
            UNREFERENCED_PARAMETER(name);
        #endif
    }

    inline uint32_t CountMips(uint32_t width, uint32_t height)
    {
        if (width == 0 || height == 0)
            return 0;

        uint32_t count = 1;
        while (width > 1 || height > 1)
        {
            width >>= 1;
            height >>= 1;
            count++;
        }
        return count;
    }

    //--------------------------------------------------------------------------------------
    HRESULT LoadTextureDataFromFile(
        _In_z_ const wchar_t* fileName,
        std::unique_ptr<uint8_t[]>& ddsData,
        const DDS_HEADER** header,
        const uint8_t** bitData,
        size_t* bitSize)
    {
        if (!header || !bitData || !bitSize)
        {
            return E_POINTER;
        }

        // open the file
        ScopedHandle hFile(safe_handle(CreateFile2(fileName,
            GENERIC_READ,
            FILE_SHARE_READ,
            OPEN_EXISTING,
            nullptr)));

        if (!hFile)
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }

        // Get the file size
        FILE_STANDARD_INFO fileInfo;
        if (!GetFileInformationByHandleEx(hFile.get(), FileStandardInfo, &fileInfo, sizeof(fileInfo)))
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }

        // File is too big for 32-bit allocation, so reject read
        if (fileInfo.EndOfFile.HighPart > 0)
        {
            return E_FAIL;
        }

        // Need at least enough data to fill the header and magic number to be a valid DDS
        if (fileInfo.EndOfFile.LowPart < (sizeof(DDS_HEADER) + sizeof(uint32_t)))
        {
            return E_FAIL;
        }

        // create enough space for the file data
        ddsData.reset(new (std::nothrow) uint8_t[fileInfo.EndOfFile.LowPart]);
        if (!ddsData)
        {
            return E_OUTOFMEMORY;
        }

        // read the data in
        DWORD BytesRead = 0;
        if (!ReadFile(hFile.get(),
            ddsData.get(),
            fileInfo.EndOfFile.LowPart,
            &BytesRead,
            nullptr
        ))
        {
            return HRESULT_FROM_WIN32(GetLastError());
        }

        if (BytesRead < fileInfo.EndOfFile.LowPart)
        {
            return E_FAIL;
        }

        // DDS files always start with the same magic number ("DDS ")
        uint32_t dwMagicNumber = *reinterpret_cast<const uint32_t*>(ddsData.get());
        if (dwMagicNumber != DDS_MAGIC)
        {
            return E_FAIL;
        }

        auto hdr = reinterpret_cast<const DDS_HEADER*>(ddsData.get() + sizeof(uint32_t));

        // Verify header to validate DDS file
        if (hdr->size != sizeof(DDS_HEADER) ||
            hdr->ddspf.size != sizeof(DDS_PIXELFORMAT))
        {
            return E_FAIL;
        }

        // Check for DX10 extension
        bool bDXT10Header = false;
        if ((hdr->ddspf.flags & DDS_FOURCC) &&
            (MAKEFOURCC('D', 'X', '1', '0') == hdr->ddspf.fourCC))
        {
            // Must be long enough for both headers and magic value
            if (fileInfo.EndOfFile.LowPart < (sizeof(DDS_HEADER) + sizeof(uint32_t) + sizeof(DDS_HEADER_DXT10)))
            {
                return E_FAIL;
            }

            bDXT10Header = true;
        }

        // setup the pointers in the process request
        *header = hdr;
        ptrdiff_t offset = sizeof(uint32_t) + sizeof(DDS_HEADER)
            + (bDXT10Header ? sizeof(DDS_HEADER_DXT10) : 0);
        *bitData = ddsData.get() + offset;
        *bitSize = fileInfo.EndOfFile.LowPart - offset;

        return S_OK;
    }


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
    // Get surface information for a particular format
    //--------------------------------------------------------------------------------------
    void GetSurfaceInfo(
        _In_ size_t width,
        _In_ size_t height,
        _In_ DXGI_FORMAT fmt,
        size_t* outNumBytes,
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
    #define ISBITMASK( r,g,b,a ) ( ddpf.RBitMask == r && ddpf.GBitMask == g && ddpf.BBitMask == b && ddpf.ABitMask == a )

    DXGI_FORMAT GetDXGIFormat( const DDS_PIXELFORMAT& ddpf )
    {
        if (ddpf.flags & DDS_RGB)
        {
            // Note that sRGB formats are written using the "DX10" extended header

            switch (ddpf.RGBBitCount)
            {
            case 32:
                if (ISBITMASK(0x000000ff,0x0000ff00,0x00ff0000,0xff000000))
                {
                    return DXGI_FORMAT_R8G8B8A8_UNORM;
                }

                if (ISBITMASK(0x00ff0000,0x0000ff00,0x000000ff,0xff000000))
                {
                    return DXGI_FORMAT_B8G8R8A8_UNORM;
                }

                if (ISBITMASK(0x00ff0000,0x0000ff00,0x000000ff,0x00000000))
                {
                    return DXGI_FORMAT_B8G8R8X8_UNORM;
                }

                // No DXGI format maps to ISBITMASK(0x000000ff,0x0000ff00,0x00ff0000,0x00000000) aka D3DFMT_X8B8G8R8

                // Note that many common DDS reader/writers (including D3DX) swap the
                // the RED/BLUE masks for 10:10:10:2 formats. We assume
                // below that the 'backwards' header mask is being used since it is most
                // likely written by D3DX. The more robust solution is to use the 'DX10'
                // header extension and specify the DXGI_FORMAT_R10G10B10A2_UNORM format directly

                // For 'correct' writers, this should be 0x000003ff,0x000ffc00,0x3ff00000 for RGB data
                if (ISBITMASK(0x3ff00000,0x000ffc00,0x000003ff,0xc0000000))
                {
                    return DXGI_FORMAT_R10G10B10A2_UNORM;
                }

                // No DXGI format maps to ISBITMASK(0x000003ff,0x000ffc00,0x3ff00000,0xc0000000) aka D3DFMT_A2R10G10B10

                if (ISBITMASK(0x0000ffff,0xffff0000,0x00000000,0x00000000))
                {
                    return DXGI_FORMAT_R16G16_UNORM;
                }

                if (ISBITMASK(0xffffffff,0x00000000,0x00000000,0x00000000))
                {
                    // Only 32-bit color channel format in D3D9 was R32F
                    return DXGI_FORMAT_R32_FLOAT; // D3DX writes this out as a FourCC of 114
                }
                break;

            case 24:
                // No 24bpp DXGI formats aka D3DFMT_R8G8B8
                break;

            case 16:
                if (ISBITMASK(0x7c00,0x03e0,0x001f,0x8000))
                {
                    return DXGI_FORMAT_B5G5R5A1_UNORM;
                }
                if (ISBITMASK(0xf800,0x07e0,0x001f,0x0000))
                {
                    return DXGI_FORMAT_B5G6R5_UNORM;
                }

                // No DXGI format maps to ISBITMASK(0x7c00,0x03e0,0x001f,0x0000) aka D3DFMT_X1R5G5B5

                if (ISBITMASK(0x0f00,0x00f0,0x000f,0xf000))
                {
                    return DXGI_FORMAT_B4G4R4A4_UNORM;
                }

                // No DXGI format maps to ISBITMASK(0x0f00,0x00f0,0x000f,0x0000) aka D3DFMT_X4R4G4B4

                // No 3:3:2, 3:3:2:8, or paletted DXGI formats aka D3DFMT_A8R3G3B2, D3DFMT_R3G3B2, D3DFMT_P8, D3DFMT_A8P8, etc.
                break;
            }
        }
        else if (ddpf.flags & DDS_LUMINANCE)
        {
            if (8 == ddpf.RGBBitCount)
            {
                if (ISBITMASK(0x000000ff,0x00000000,0x00000000,0x00000000))
                {
                    return DXGI_FORMAT_R8_UNORM; // D3DX10/11 writes this out as DX10 extension
                }

                // No DXGI format maps to ISBITMASK(0x0f,0x00,0x00,0xf0) aka D3DFMT_A4L4

                if (ISBITMASK(0x000000ff, 0x00000000, 0x00000000, 0x0000ff00))
                {
                    return DXGI_FORMAT_R8G8_UNORM; // Some DDS writers assume the bitcount should be 8 instead of 16
                }
            }

            if (16 == ddpf.RGBBitCount)
            {
                if (ISBITMASK(0x0000ffff,0x00000000,0x00000000,0x00000000))
                {
                    return DXGI_FORMAT_R16_UNORM; // D3DX10/11 writes this out as DX10 extension
                }
                if (ISBITMASK(0x000000ff,0x00000000,0x00000000,0x0000ff00))
                {
                    return DXGI_FORMAT_R8G8_UNORM; // D3DX10/11 writes this out as DX10 extension
                }
            }
        }
        else if (ddpf.flags & DDS_ALPHA)
        {
            if (8 == ddpf.RGBBitCount)
            {
                return DXGI_FORMAT_A8_UNORM;
            }
        }
        else if (ddpf.flags & DDS_BUMPDUDV)
        {
            if (16 == ddpf.RGBBitCount)
            {
                if (ISBITMASK(0x00ff, 0xff00, 0x0000, 0x0000))
                {
                    return DXGI_FORMAT_R8G8_SNORM; // D3DX10/11 writes this out as DX10 extension
                }
            }

            if (32 == ddpf.RGBBitCount)
            {
                if (ISBITMASK(0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000))
                {
                    return DXGI_FORMAT_R8G8B8A8_SNORM; // D3DX10/11 writes this out as DX10 extension
                }
                if (ISBITMASK(0x0000ffff, 0xffff0000, 0x00000000, 0x00000000))
                {
                    return DXGI_FORMAT_R16G16_SNORM; // D3DX10/11 writes this out as DX10 extension
                }

                // No DXGI format maps to ISBITMASK(0x3ff00000, 0x000ffc00, 0x000003ff, 0xc0000000) aka D3DFMT_A2W10V10U10
            }
        }
        else if (ddpf.flags & DDS_FOURCC)
        {
            if (MAKEFOURCC( 'D', 'X', 'T', '1' ) == ddpf.fourCC)
            {
                return DXGI_FORMAT_BC1_UNORM;
            }
            if (MAKEFOURCC( 'D', 'X', 'T', '3' ) == ddpf.fourCC)
            {
                return DXGI_FORMAT_BC2_UNORM;
            }
            if (MAKEFOURCC( 'D', 'X', 'T', '5' ) == ddpf.fourCC)
            {
                return DXGI_FORMAT_BC3_UNORM;
            }

            // While pre-multiplied alpha isn't directly supported by the DXGI formats,
            // they are basically the same as these BC formats so they can be mapped
            if (MAKEFOURCC( 'D', 'X', 'T', '2' ) == ddpf.fourCC)
            {
                return DXGI_FORMAT_BC2_UNORM;
            }
            if (MAKEFOURCC( 'D', 'X', 'T', '4' ) == ddpf.fourCC)
            {
                return DXGI_FORMAT_BC3_UNORM;
            }

            if (MAKEFOURCC( 'A', 'T', 'I', '1' ) == ddpf.fourCC)
            {
                return DXGI_FORMAT_BC4_UNORM;
            }
            if (MAKEFOURCC( 'B', 'C', '4', 'U' ) == ddpf.fourCC)
            {
                return DXGI_FORMAT_BC4_UNORM;
            }
            if (MAKEFOURCC( 'B', 'C', '4', 'S' ) == ddpf.fourCC)
            {
                return DXGI_FORMAT_BC4_SNORM;
            }

            if (MAKEFOURCC( 'A', 'T', 'I', '2' ) == ddpf.fourCC)
            {
                return DXGI_FORMAT_BC5_UNORM;
            }
            if (MAKEFOURCC( 'B', 'C', '5', 'U' ) == ddpf.fourCC)
            {
                return DXGI_FORMAT_BC5_UNORM;
            }
            if (MAKEFOURCC( 'B', 'C', '5', 'S' ) == ddpf.fourCC)
            {
                return DXGI_FORMAT_BC5_SNORM;
            }

            // BC6H and BC7 are written using the "DX10" extended header

            if (MAKEFOURCC( 'R', 'G', 'B', 'G' ) == ddpf.fourCC)
            {
                return DXGI_FORMAT_R8G8_B8G8_UNORM;
            }
            if (MAKEFOURCC( 'G', 'R', 'G', 'B' ) == ddpf.fourCC)
            {
                return DXGI_FORMAT_G8R8_G8B8_UNORM;
            }

            if (MAKEFOURCC('Y','U','Y','2') == ddpf.fourCC)
            {
                return DXGI_FORMAT_YUY2;
            }

            // Check for D3DFORMAT enums being set here
            switch( ddpf.fourCC )
            {
            case 36: // D3DFMT_A16B16G16R16
                return DXGI_FORMAT_R16G16B16A16_UNORM;

            case 110: // D3DFMT_Q16W16V16U16
                return DXGI_FORMAT_R16G16B16A16_SNORM;

            case 111: // D3DFMT_R16F
                return DXGI_FORMAT_R16_FLOAT;

            case 112: // D3DFMT_G16R16F
                return DXGI_FORMAT_R16G16_FLOAT;

            case 113: // D3DFMT_A16B16G16R16F
                return DXGI_FORMAT_R16G16B16A16_FLOAT;

            case 114: // D3DFMT_R32F
                return DXGI_FORMAT_R32_FLOAT;

            case 115: // D3DFMT_G32R32F
                return DXGI_FORMAT_R32G32_FLOAT;

            case 116: // D3DFMT_A32B32G32R32F
                return DXGI_FORMAT_R32G32B32A32_FLOAT;
            }
        }

        return DXGI_FORMAT_UNKNOWN;
    }


    //--------------------------------------------------------------------------------------
    DXGI_FORMAT MakeSRGB( _In_ DXGI_FORMAT format )
    {
        switch( format )
        {
        case DXGI_FORMAT_R8G8B8A8_UNORM:
            return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

        case DXGI_FORMAT_BC1_UNORM:
            return DXGI_FORMAT_BC1_UNORM_SRGB;

        case DXGI_FORMAT_BC2_UNORM:
            return DXGI_FORMAT_BC2_UNORM_SRGB;

        case DXGI_FORMAT_BC3_UNORM:
            return DXGI_FORMAT_BC3_UNORM_SRGB;

        case DXGI_FORMAT_B8G8R8A8_UNORM:
            return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;

        case DXGI_FORMAT_B8G8R8X8_UNORM:
            return DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;

        case DXGI_FORMAT_BC7_UNORM:
            return DXGI_FORMAT_BC7_UNORM_SRGB;

        default:
            return format;
        }
    }


    //--------------------------------------------------------------------------------------
    inline bool IsDepthStencil(DXGI_FORMAT fmt)
    {
        switch (fmt)
        {
        case DXGI_FORMAT_R32G8X24_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
        case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
        case DXGI_FORMAT_D32_FLOAT:
        case DXGI_FORMAT_R24G8_TYPELESS:
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
        case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
        case DXGI_FORMAT_D16_UNORM:
            return true;

        default:
            return false;
        }
    }


    //--------------------------------------------------------------------------------------
    inline void AdjustPlaneResource(
        _In_ DXGI_FORMAT fmt,
        _In_ size_t height,
        _In_ size_t slicePlane,
        _Inout_ D3D12_SUBRESOURCE_DATA& res)
    {
        switch (fmt)
        {
        case DXGI_FORMAT_NV12:
        case DXGI_FORMAT_P010:
        case DXGI_FORMAT_P016:

#if defined(_XBOX_ONE) && defined(_TITLE)
        case DXGI_FORMAT_D16_UNORM_S8_UINT:
        case DXGI_FORMAT_R16_UNORM_X8_TYPELESS:
        case DXGI_FORMAT_X16_TYPELESS_G8_UINT:
#endif
            if (!slicePlane)
            {
                // Plane 0
                res.SlicePitch = res.RowPitch * height;
            }
            else
            {
                // Plane 1
                res.pData = reinterpret_cast<const uint8_t*>(res.pData) + res.RowPitch * height;
                res.SlicePitch = res.RowPitch * ((height + 1) >> 1);
            }
            break;

        case DXGI_FORMAT_NV11:
            if (!slicePlane)
            {
                // Plane 0
                res.SlicePitch = res.RowPitch * height;
            }
            else
            {
                // Plane 1
                res.pData = reinterpret_cast<const uint8_t*>(res.pData) + res.RowPitch * height;
                res.RowPitch = (res.RowPitch >> 1);
                res.SlicePitch = res.RowPitch * height;
            }
            break;
        }
    }


    //--------------------------------------------------------------------------------------
    HRESULT FillInitData(_In_ size_t width,
        _In_ size_t height,
        _In_ size_t depth,
        _In_ size_t mipCount,
        _In_ size_t arraySize,
        _In_ size_t numberOfPlanes,
        _In_ DXGI_FORMAT format,
        _In_ size_t maxsize,
        _In_ size_t bitSize,
        _In_reads_bytes_(bitSize) const uint8_t* bitData,
        _Out_ size_t& twidth,
        _Out_ size_t& theight,
        _Out_ size_t& tdepth,
        _Out_ size_t& skipMip,
        std::vector<D3D12_SUBRESOURCE_DATA>& initData)
    {
        if (!bitData)
        {
            return E_POINTER;
        }

        skipMip = 0;
        twidth = 0;
        theight = 0;
        tdepth = 0;

        size_t NumBytes = 0;
        size_t RowBytes = 0;
        const uint8_t* pEndBits = bitData + bitSize;

        initData.clear();

        for (size_t p = 0; p < numberOfPlanes; ++p)
        {
            const uint8_t* pSrcBits = bitData;

            for (size_t j = 0; j < arraySize; j++)
            {
                size_t w = width;
                size_t h = height;
                size_t d = depth;
                for (size_t i = 0; i < mipCount; i++)
                {
                    GetSurfaceInfo(w,
                        h,
                        format,
                        &NumBytes,
                        &RowBytes,
                        nullptr
                    );

                    if ((mipCount <= 1) || !maxsize || (w <= maxsize && h <= maxsize && d <= maxsize))
                    {
                        if (!twidth)
                        {
                            twidth = w;
                            theight = h;
                            tdepth = d;
                        }

                        D3D12_SUBRESOURCE_DATA res =
                        {
                            reinterpret_cast<const void*>(pSrcBits),
                            static_cast<LONG_PTR>(RowBytes),
                            static_cast<LONG_PTR>(NumBytes)
                        };

                        AdjustPlaneResource(format, h, p, res);

                        initData.emplace_back(res);
                    }
                    else if (!j)
                    {
                        // Count number of skipped mipmaps (first item only)
                        ++skipMip;
                    }

                    if (pSrcBits + (NumBytes*d) > pEndBits)
                    {
                        return HRESULT_FROM_WIN32(ERROR_HANDLE_EOF);
                    }

                    pSrcBits += NumBytes * d;

                    w = w >> 1;
                    h = h >> 1;
                    d = d >> 1;
                    if (w == 0)
                    {
                        w = 1;
                    }
                    if (h == 0)
                    {
                        h = 1;
                    }
                    if (d == 0)
                    {
                        d = 1;
                    }
                }
            }
        }

        return initData.empty() ? E_FAIL : S_OK;
    }


    //--------------------------------------------------------------------------------------
    HRESULT CreateTextureResource(
        _In_ ID3D12Device* d3dDevice,
        D3D12_RESOURCE_DIMENSION resDim,
        size_t width,
        size_t height,
        size_t depth,
        size_t mipCount,
        size_t arraySize,
        DXGI_FORMAT format,
        D3D12_RESOURCE_FLAGS resFlags,
        unsigned int loadFlags,
        _Outptr_ ID3D12Resource** texture)
    {
        if (!d3dDevice)
            return E_POINTER;

        HRESULT hr = E_FAIL;

        if (loadFlags & DDS_LOADER_FORCE_SRGB)
        {
            format = MakeSRGB(format);
        }

        D3D12_RESOURCE_DESC desc = {};
        desc.Width = static_cast<UINT>(width);
        desc.Height = static_cast<UINT>(height);
        desc.MipLevels = static_cast<UINT16>(mipCount);
        desc.DepthOrArraySize = (resDim == D3D12_RESOURCE_DIMENSION_TEXTURE3D) ? static_cast<UINT16>(depth) : static_cast<UINT16>(arraySize);
        desc.Format = format;
        desc.Flags = resFlags;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.Dimension = resDim;

        CD3DX12_HEAP_PROPERTIES defaultHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

        hr = d3dDevice->CreateCommittedResource(
            &defaultHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(texture));
        if (SUCCEEDED(hr))
        {
            _Analysis_assume_(*texture != 0);

            SetDebugObjectName(*texture, L"DDSTextureLoader");
        }

        return hr;
    }

    //--------------------------------------------------------------------------------------
    HRESULT CreateTextureFromDDS(_In_ ID3D12Device* d3dDevice,
        _In_ const DDS_HEADER* header,
        _In_reads_bytes_(bitSize) const uint8_t* bitData,
        size_t bitSize,
        size_t maxsize,
        D3D12_RESOURCE_FLAGS resFlags,
        unsigned int loadFlags,
        _Outptr_ ID3D12Resource** texture,
        std::vector<D3D12_SUBRESOURCE_DATA>& subresources,
        _Out_opt_ bool* outIsCubeMap)
    {
        HRESULT hr = S_OK;

        UINT width = header->width;
        UINT height = header->height;
        UINT depth = header->depth;

        D3D12_RESOURCE_DIMENSION resDim = D3D12_RESOURCE_DIMENSION_UNKNOWN;
        UINT arraySize = 1;
        DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
        bool isCubeMap = false;

        size_t mipCount = header->mipMapCount;
        if (0 == mipCount)
        {
            mipCount = 1;
        }

        if ((header->ddspf.flags & DDS_FOURCC) &&
            (MAKEFOURCC('D', 'X', '1', '0') == header->ddspf.fourCC))
        {
            auto d3d10ext = reinterpret_cast<const DDS_HEADER_DXT10*>((const char*)header + sizeof(DDS_HEADER));

            arraySize = d3d10ext->arraySize;
            if (arraySize == 0)
            {
                return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            }

            switch (d3d10ext->dxgiFormat)
            {
            case DXGI_FORMAT_AI44:
            case DXGI_FORMAT_IA44:
            case DXGI_FORMAT_P8:
            case DXGI_FORMAT_A8P8:
                return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);

            default:
                if (BitsPerPixel(d3d10ext->dxgiFormat) == 0)
                {
                    return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
                }
            }

            format = d3d10ext->dxgiFormat;

            switch (d3d10ext->resourceDimension)
            {
            case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
                // D3DX writes 1D textures with a fixed Height of 1
                if ((header->flags & DDS_HEIGHT) && height != 1)
                {
                    return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                }
                height = depth = 1;
                break;

            case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
                if (d3d10ext->miscFlag & 0x4 /* RESOURCE_MISC_TEXTURECUBE */)
                {
                    arraySize *= 6;
                    isCubeMap = true;
                }
                depth = 1;
                break;

            case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
                if (!(header->flags & DDS_HEADER_FLAGS_VOLUME))
                {
                    return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                }

                if (arraySize > 1)
                {
                    return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
                }
                break;

            default:
                return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
            }

            resDim = static_cast<D3D12_RESOURCE_DIMENSION>(d3d10ext->resourceDimension);
        }
        else
        {
            format = GetDXGIFormat(header->ddspf);

            if (format == DXGI_FORMAT_UNKNOWN)
            {
                return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
            }

            if (header->flags & DDS_HEADER_FLAGS_VOLUME)
            {
                resDim = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
            }
            else
            {
                if (header->caps2 & DDS_CUBEMAP)
                {
                    // We require all six faces to be defined
                    if ((header->caps2 & DDS_CUBEMAP_ALLFACES) != DDS_CUBEMAP_ALLFACES)
                    {
                        return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
                    }

                    arraySize = 6;
                    isCubeMap = true;
                }

                depth = 1;
                resDim = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

                // Note there's no way for a legacy Direct3D 9 DDS to express a '1D' texture
            }

            assert(BitsPerPixel(format) != 0);
        }

        // Bound sizes (for security purposes we don't trust DDS file metadata larger than the Direct3D hardware requirements)
        if (mipCount > D3D12_REQ_MIP_LEVELS)
        {
            return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
        }

        switch (resDim)
        {
        case D3D12_RESOURCE_DIMENSION_TEXTURE1D:
            if ((arraySize > D3D12_REQ_TEXTURE1D_ARRAY_AXIS_DIMENSION) ||
                (width > D3D12_REQ_TEXTURE1D_U_DIMENSION))
            {
                return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
            }
            break;

        case D3D12_RESOURCE_DIMENSION_TEXTURE2D:
            if (isCubeMap)
            {
                // This is the right bound because we set arraySize to (NumCubes*6) above
                if ((arraySize > D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION) ||
                    (width > D3D12_REQ_TEXTURECUBE_DIMENSION) ||
                    (height > D3D12_REQ_TEXTURECUBE_DIMENSION))
                {
                    return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
                }
            }
            else if ((arraySize > D3D12_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION) ||
                (width > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION) ||
                (height > D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION))
            {
                return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
            }
            break;

        case D3D12_RESOURCE_DIMENSION_TEXTURE3D:
            if ((arraySize > 1) ||
                (width > D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION) ||
                (height > D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION) ||
                (depth > D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION))
            {
                return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
            }
            break;

        default:
            return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
        }

        UINT numberOfPlanes = D3D12GetFormatPlaneCount(d3dDevice, format);
        if (!numberOfPlanes)
            return E_INVALIDARG;

        if ((numberOfPlanes > 1) && IsDepthStencil(format))
        {
            // DirectX 12 uses planes for stencil, DirectX 11 does not
            return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
        }

        if (outIsCubeMap != nullptr)
        {
            *outIsCubeMap = isCubeMap;
        }

        // Create the texture
        size_t numberOfResources = (resDim == D3D12_RESOURCE_DIMENSION_TEXTURE3D)
                                   ? 1 : arraySize;
        numberOfResources *= mipCount;
        numberOfResources *= numberOfPlanes;

        if (numberOfResources > D3D12_REQ_SUBRESOURCES)
            return E_INVALIDARG;

        subresources.reserve(numberOfResources);

        size_t skipMip = 0;
        size_t twidth = 0;
        size_t theight = 0;
        size_t tdepth = 0;
        hr = FillInitData(width, height, depth, mipCount, arraySize,
            numberOfPlanes, format,
            maxsize, bitSize, bitData,
            twidth, theight, tdepth, skipMip, subresources);

        if (SUCCEEDED(hr))
        {
            size_t reservedMips = mipCount;
            if (loadFlags & DDS_LOADER_MIP_RESERVE)
            {
                reservedMips = std::min<size_t>(D3D12_REQ_MIP_LEVELS, CountMips(width, height));
            }

            hr = CreateTextureResource(d3dDevice, resDim, twidth, theight, tdepth, reservedMips - skipMip, arraySize,
                format, resFlags, loadFlags, texture);

            if (FAILED(hr) && !maxsize && (mipCount > 1))
            {
                subresources.clear();

                maxsize = (resDim == D3D12_RESOURCE_DIMENSION_TEXTURE3D)
                    ? D3D12_REQ_TEXTURE3D_U_V_OR_W_DIMENSION
                    : D3D12_REQ_TEXTURE2D_U_OR_V_DIMENSION;

                hr = FillInitData(width, height, depth, mipCount, arraySize,
                    numberOfPlanes, format,
                    maxsize, bitSize, bitData,
                    twidth, theight, tdepth, skipMip, subresources);
                if (SUCCEEDED(hr))
                {
                    hr = CreateTextureResource(d3dDevice, resDim, twidth, theight, tdepth, mipCount - skipMip, arraySize,
                        format, resFlags, loadFlags, texture);
                }
            }
        }

        if (FAILED(hr))
        {
            subresources.clear();
        }

        return hr;
    }

    //--------------------------------------------------------------------------------------
    DDS_ALPHA_MODE GetAlphaMode( _In_ const DDS_HEADER* header )
    {
        if ( header->ddspf.flags & DDS_FOURCC )
        {
            if ( MAKEFOURCC( 'D', 'X', '1', '0' ) == header->ddspf.fourCC )
            {
                auto d3d10ext = reinterpret_cast<const DDS_HEADER_DXT10*>( (const char*)header + sizeof(DDS_HEADER) );
                auto mode = static_cast<DDS_ALPHA_MODE>( d3d10ext->miscFlags2 & DDS_MISC_FLAGS2_ALPHA_MODE_MASK );
                switch( mode )
                {
                case DDS_ALPHA_MODE_STRAIGHT:
                case DDS_ALPHA_MODE_PREMULTIPLIED:
                case DDS_ALPHA_MODE_OPAQUE:
                case DDS_ALPHA_MODE_CUSTOM:
                    return mode;
                }
            }
            else if ( ( MAKEFOURCC( 'D', 'X', 'T', '2' ) == header->ddspf.fourCC )
                      || ( MAKEFOURCC( 'D', 'X', 'T', '4' ) == header->ddspf.fourCC ) )
            {
                return DDS_ALPHA_MODE_PREMULTIPLIED;
            }
        }

        return DDS_ALPHA_MODE_UNKNOWN;
    }
} // anonymous namespace


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT DirectX::LoadDDSTextureFromMemory(
    ID3D12Device* d3dDevice,
    const uint8_t* ddsData,
    size_t ddsDataSize,
    ID3D12Resource** texture,
    std::vector<D3D12_SUBRESOURCE_DATA>& subresources,
    size_t maxsize,
    DDS_ALPHA_MODE* alphaMode,
    bool* isCubeMap)
{
    return LoadDDSTextureFromMemoryEx(
        d3dDevice,
        ddsData,
        ddsDataSize,
        maxsize,
        D3D12_RESOURCE_FLAG_NONE,
        DDS_LOADER_DEFAULT,
        texture,
        subresources,
        alphaMode,
        isCubeMap);
}


_Use_decl_annotations_
HRESULT DirectX::LoadDDSTextureFromMemoryEx(
    ID3D12Device* d3dDevice,
    const uint8_t* ddsData,
    size_t ddsDataSize,
    size_t maxsize,
    D3D12_RESOURCE_FLAGS resFlags,
    unsigned int loadFlags,
    ID3D12Resource** texture,
    std::vector<D3D12_SUBRESOURCE_DATA>& subresources,
    DDS_ALPHA_MODE* alphaMode,
    bool* isCubeMap)
{
    if (texture)
    {
        *texture = nullptr;
    }
    if (alphaMode)
    {
        *alphaMode = DDS_ALPHA_MODE_UNKNOWN;
    }
    if (isCubeMap)
    {
        *isCubeMap = false;
    }

    if (!d3dDevice || !ddsData || !texture)
    {
        return E_INVALIDARG;
    }

    // Validate DDS file in memory
    if (ddsDataSize < (sizeof(uint32_t) + sizeof(DDS_HEADER)))
    {
        return E_FAIL;
    }

    uint32_t dwMagicNumber = *(const uint32_t*)(ddsData);
    if (dwMagicNumber != DDS_MAGIC)
    {
        return E_FAIL;
    }

    auto header = reinterpret_cast<const DDS_HEADER*>(ddsData + sizeof(uint32_t));

    // Verify header to validate DDS file
    if (header->size != sizeof(DDS_HEADER) ||
        header->ddspf.size != sizeof(DDS_PIXELFORMAT))
    {
        return E_FAIL;
    }

    // Check for DX10 extension
    bool bDXT10Header = false;
    if ((header->ddspf.flags & DDS_FOURCC) &&
        (MAKEFOURCC('D', 'X', '1', '0') == header->ddspf.fourCC))
    {
        // Must be long enough for both headers and magic value
        if (ddsDataSize < (sizeof(DDS_HEADER) + sizeof(uint32_t) + sizeof(DDS_HEADER_DXT10)))
        {
            return E_FAIL;
        }

        bDXT10Header = true;
    }

    ptrdiff_t offset = sizeof(uint32_t)
        + sizeof(DDS_HEADER)
        + (bDXT10Header ? sizeof(DDS_HEADER_DXT10) : 0);

    HRESULT hr = CreateTextureFromDDS(d3dDevice,
        header, ddsData + offset, ddsDataSize - offset, maxsize,
        resFlags, loadFlags,
        texture, subresources, isCubeMap);
    if (SUCCEEDED(hr))
    {
        if (texture != 0 && *texture != 0)
        {
            SetDebugObjectName(*texture, L"DDSTextureLoader");
        }

        if (alphaMode)
            *alphaMode = GetAlphaMode(header);
    }

    return hr;
}


//--------------------------------------------------------------------------------------
_Use_decl_annotations_
HRESULT DirectX::LoadDDSTextureFromFile(
    ID3D12Device* d3dDevice,
    const wchar_t* fileName,
    ID3D12Resource** texture,
    std::unique_ptr<uint8_t[]>& ddsData,
    std::vector<D3D12_SUBRESOURCE_DATA>& subresources,
    size_t maxsize,
    DDS_ALPHA_MODE* alphaMode,
    bool* isCubeMap)
{
    return LoadDDSTextureFromFileEx(
        d3dDevice,
        fileName,
        maxsize,
        D3D12_RESOURCE_FLAG_NONE,
        DDS_LOADER_DEFAULT,
        texture,
        ddsData,
        subresources,
        alphaMode,
        isCubeMap);
}

_Use_decl_annotations_
HRESULT DirectX::LoadDDSTextureFromFileEx(
    ID3D12Device* d3dDevice,
    const wchar_t* fileName,
    size_t maxsize,
    D3D12_RESOURCE_FLAGS resFlags,
    unsigned int loadFlags,
    ID3D12Resource** texture,
    std::unique_ptr<uint8_t[]>& ddsData,
    std::vector<D3D12_SUBRESOURCE_DATA>& subresources,
    DDS_ALPHA_MODE* alphaMode,
    bool* isCubeMap)
{
    if (texture)
    {
        *texture = nullptr;
    }
    if (alphaMode)
    {
        *alphaMode = DDS_ALPHA_MODE_UNKNOWN;
    }
    if (isCubeMap)
    {
        *isCubeMap = false;
    }

    if (!d3dDevice || !fileName || !texture)
    {
        return E_INVALIDARG;
    }

    const DDS_HEADER* header = nullptr;
    const uint8_t* bitData = nullptr;
    size_t bitSize = 0;

    HRESULT hr = LoadTextureDataFromFile(fileName,
        ddsData,
        &header,
        &bitData,
        &bitSize
    );
    if (FAILED(hr))
    {
        return hr;
    }

    hr = CreateTextureFromDDS(d3dDevice,
        header, bitData, bitSize, maxsize,
        resFlags, loadFlags,
        texture, subresources, isCubeMap);

    if (SUCCEEDED(hr))
    {
#if !defined(NO_D3D12_DEBUG_NAME) && ( defined(_DEBUG) || defined(PROFILE) )
        if (texture != 0)
        {
            CHAR strFileA[MAX_PATH];
            int result = WideCharToMultiByte(CP_ACP,
                WC_NO_BEST_FIT_CHARS,
                fileName,
                -1,
                strFileA,
                MAX_PATH,
                nullptr,
                FALSE
            );
            if (result > 0)
            {
                const wchar_t* pstrName = wcsrchr(fileName, '\\');
                if (!pstrName)
                {
                    pstrName = fileName;
                }
                else
                {
                    pstrName++;
                }

                if (texture != 0 && *texture != 0)
                {
                    (*texture)->SetName(pstrName);
                }
            }
        }
#endif

        if (alphaMode)
            *alphaMode = GetAlphaMode(header);
    }

    return hr;
}
