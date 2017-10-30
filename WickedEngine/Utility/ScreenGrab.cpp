//--------------------------------------------------------------------------------------
// File: ScreenGrab.cpp
//
// Function for capturing a 2D texture and saving it to a file (aka a 'screenshot'
// when used on a Direct3D 11 Render Target).
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
// http://go.microsoft.com/fwlink/?LinkId=248929
//--------------------------------------------------------------------------------------

// Does not capture 1D textures or 3D textures (volume maps)

// Does not capture mipmap chains, only the top-most texture level is saved

// For 2D array textures and cubemaps, it captures only the first image in the array

#include <dxgiformat.h>
#include <assert.h>
#include <algorithm>

#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY != WINAPI_FAMILY_PHONE_APP)
// VS 2010's stdint.h conflicts with intsafe.h
#pragma warning(push)
#pragma warning(disable : 4005)
#include <wincodec.h>
#include <wrl.h>
#pragma warning(pop)
#endif

#include <memory>

#include "ScreenGrab.h"

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

static const DDS_PIXELFORMAT DDSPF_DXT1 =
    { sizeof(DDS_PIXELFORMAT), DDS_FOURCC, MAKEFOURCC('D','X','T','1'), 0, 0, 0, 0, 0 };

static const DDS_PIXELFORMAT DDSPF_DXT3 =
    { sizeof(DDS_PIXELFORMAT), DDS_FOURCC, MAKEFOURCC('D','X','T','3'), 0, 0, 0, 0, 0 };

static const DDS_PIXELFORMAT DDSPF_DXT5 =
    { sizeof(DDS_PIXELFORMAT), DDS_FOURCC, MAKEFOURCC('D','X','T','5'), 0, 0, 0, 0, 0 };

static const DDS_PIXELFORMAT DDSPF_BC4_UNORM =
    { sizeof(DDS_PIXELFORMAT), DDS_FOURCC, MAKEFOURCC('B','C','4','U'), 0, 0, 0, 0, 0 };

static const DDS_PIXELFORMAT DDSPF_BC4_SNORM =
    { sizeof(DDS_PIXELFORMAT), DDS_FOURCC, MAKEFOURCC('B','C','4','S'), 0, 0, 0, 0, 0 };

static const DDS_PIXELFORMAT DDSPF_BC5_UNORM =
    { sizeof(DDS_PIXELFORMAT), DDS_FOURCC, MAKEFOURCC('B','C','5','U'), 0, 0, 0, 0, 0 };

static const DDS_PIXELFORMAT DDSPF_BC5_SNORM =
    { sizeof(DDS_PIXELFORMAT), DDS_FOURCC, MAKEFOURCC('B','C','5','S'), 0, 0, 0, 0, 0 };

static const DDS_PIXELFORMAT DDSPF_R8G8_B8G8 =
    { sizeof(DDS_PIXELFORMAT), DDS_FOURCC, MAKEFOURCC('R','G','B','G'), 0, 0, 0, 0, 0 };

static const DDS_PIXELFORMAT DDSPF_G8R8_G8B8 =
    { sizeof(DDS_PIXELFORMAT), DDS_FOURCC, MAKEFOURCC('G','R','G','B'), 0, 0, 0, 0, 0 };

static const DDS_PIXELFORMAT DDSPF_A8R8G8B8 =
    { sizeof(DDS_PIXELFORMAT), DDS_RGBA, 0, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000 };

static const DDS_PIXELFORMAT DDSPF_X8R8G8B8 =
    { sizeof(DDS_PIXELFORMAT), DDS_RGB,  0, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000 };

static const DDS_PIXELFORMAT DDSPF_A8B8G8R8 =
    { sizeof(DDS_PIXELFORMAT), DDS_RGBA, 0, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000 };

static const DDS_PIXELFORMAT DDSPF_G16R16 =
    { sizeof(DDS_PIXELFORMAT), DDS_RGB,  0, 32, 0x0000ffff, 0xffff0000, 0x00000000, 0x00000000 };

static const DDS_PIXELFORMAT DDSPF_R5G6B5 =
    { sizeof(DDS_PIXELFORMAT), DDS_RGB, 0, 16, 0x0000f800, 0x000007e0, 0x0000001f, 0x00000000 };

static const DDS_PIXELFORMAT DDSPF_A1R5G5B5 =
    { sizeof(DDS_PIXELFORMAT), DDS_RGBA, 0, 16, 0x00007c00, 0x000003e0, 0x0000001f, 0x00008000 };

static const DDS_PIXELFORMAT DDSPF_A4R4G4B4 =
    { sizeof(DDS_PIXELFORMAT), DDS_RGBA, 0, 16, 0x00000f00, 0x000000f0, 0x0000000f, 0x0000f000 };

static const DDS_PIXELFORMAT DDSPF_L8 =
    { sizeof(DDS_PIXELFORMAT), DDS_LUMINANCE, 0,  8, 0xff, 0x00, 0x00, 0x00 };

static const DDS_PIXELFORMAT DDSPF_L16 =
    { sizeof(DDS_PIXELFORMAT), DDS_LUMINANCE, 0, 16, 0xffff, 0x0000, 0x0000, 0x0000 };

static const DDS_PIXELFORMAT DDSPF_A8L8 =
    { sizeof(DDS_PIXELFORMAT), DDS_LUMINANCEA, 0, 16, 0x00ff, 0x0000, 0x0000, 0xff00 };

static const DDS_PIXELFORMAT DDSPF_A8 =
    { sizeof(DDS_PIXELFORMAT), DDS_ALPHA, 0, 8, 0x00, 0x00, 0x00, 0xff };

// DXGI_FORMAT_R10G10B10A2_UNORM should be written using DX10 extension to avoid D3DX 10:10:10:2 reversal issue

// This indicates the DDS_HEADER_DXT10 extension is present (the format is in dxgiFormat)
static const DDS_PIXELFORMAT DDSPF_DX10 =
    { sizeof(DDS_PIXELFORMAT), DDS_FOURCC, MAKEFOURCC('D','X','1','0'), 0, 0, 0, 0, 0 };

//---------------------------------------------------------------------------------
struct handle_closer { void operator()(HANDLE h) { if (h) CloseHandle(h); } };

typedef public std::unique_ptr<void, handle_closer> ScopedHandle;

inline HANDLE safe_handle( HANDLE h ) { return (h == INVALID_HANDLE_VALUE) ? 0 : h; }


//--------------------------------------------------------------------------------------
// Return the BPP for a particular format
//--------------------------------------------------------------------------------------
static size_t BitsPerPixel( _In_ DXGI_FORMAT fmt )
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
        return 32;

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
    case DXGI_FORMAT_B4G4R4A4_UNORM:
        return 16;

    case DXGI_FORMAT_R8_TYPELESS:
    case DXGI_FORMAT_R8_UNORM:
    case DXGI_FORMAT_R8_UINT:
    case DXGI_FORMAT_R8_SNORM:
    case DXGI_FORMAT_R8_SINT:
    case DXGI_FORMAT_A8_UNORM:
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
static bool IsCompressed( _In_ DXGI_FORMAT fmt )
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
static void GetSurfaceInfo( _In_ size_t width,
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
    bool packed  = false;
    size_t bcnumBytesPerBlock = 0;
    switch (fmt)
    {
    case DXGI_FORMAT_BC1_TYPELESS:
    case DXGI_FORMAT_BC1_UNORM:
    case DXGI_FORMAT_BC1_UNORM_SRGB:
    case DXGI_FORMAT_BC4_TYPELESS:
    case DXGI_FORMAT_BC4_UNORM:
    case DXGI_FORMAT_BC4_SNORM:
        bc=true;
        bcnumBytesPerBlock = 8;
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
        bcnumBytesPerBlock = 16;
        break;

    case DXGI_FORMAT_R8G8_B8G8_UNORM:
    case DXGI_FORMAT_G8R8_G8B8_UNORM:
        packed = true;
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
        rowBytes = numBlocksWide * bcnumBytesPerBlock;
        numRows = numBlocksHigh;
    }
    else if (packed)
    {
        rowBytes = ( ( width + 1 ) >> 1 ) * 4;
        numRows = height;
    }
    else
    {
        size_t bpp = BitsPerPixel( fmt );
        rowBytes = ( width * bpp + 7 ) / 8; // round up to nearest byte
        numRows = height;
    }

    numBytes = rowBytes * numRows;
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
static DXGI_FORMAT EnsureNotTypeless( DXGI_FORMAT fmt )
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
static HRESULT CaptureTexture( _In_ ID3D11DeviceContext* pContext,
                               _In_ ID3D11Resource* pSource,
                               _Inout_ D3D11_TEXTURE2D_DESC& desc,
                               _Inout_ ComPtr<ID3D11Texture2D>& pStaging )
{
    if ( !pContext || !pSource )
        return E_INVALIDARG;

    D3D11_RESOURCE_DIMENSION resType = D3D11_RESOURCE_DIMENSION_UNKNOWN;
    pSource->GetType( &resType );

    if ( resType != D3D11_RESOURCE_DIMENSION_TEXTURE2D )
        return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );

    ComPtr<ID3D11Texture2D> pTexture;
    HRESULT hr = pSource->QueryInterface( __uuidof(ID3D11Texture2D), reinterpret_cast<void**>( pTexture.GetAddressOf() ) );
    if ( FAILED(hr) )
        return hr;

    assert( pTexture );

    pTexture->GetDesc( &desc );

    ComPtr<ID3D11Device> d3dDevice;
    pContext->GetDevice( d3dDevice.GetAddressOf() );

    if ( desc.SampleDesc.Count > 1 )
    {
        // MSAA content must be resolved before being copied to a staging texture
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;

        ComPtr<ID3D11Texture2D> pTemp;
        hr = d3dDevice->CreateTexture2D( &desc, 0, pTemp.GetAddressOf() );
        if ( FAILED(hr) )
            return hr;

        assert( pTemp );

        DXGI_FORMAT fmt = EnsureNotTypeless( desc.Format );

        UINT support = 0;
        hr = d3dDevice->CheckFormatSupport( fmt, &support );
        if ( FAILED(hr) )
            return hr;

        if ( !(support & D3D11_FORMAT_SUPPORT_MULTISAMPLE_RESOLVE) )
            return E_FAIL;

        for( UINT item = 0; item < desc.ArraySize; ++item )
        {
            for( UINT level = 0; level < desc.MipLevels; ++level )
            {
                UINT index = D3D11CalcSubresource( level, item, desc.MipLevels );
                pContext->ResolveSubresource( pTemp.Get(), index, pSource, index, fmt );
            }
        }

        desc.BindFlags = 0;
        desc.MiscFlags &= D3D11_RESOURCE_MISC_TEXTURECUBE;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        desc.Usage = D3D11_USAGE_STAGING;

        hr = d3dDevice->CreateTexture2D( &desc, 0, pStaging.GetAddressOf() );
        if ( FAILED(hr) )
            return hr;

        assert( pStaging );

        pContext->CopyResource( pStaging.Get(), pTemp.Get() );
    }
    else if ( (desc.Usage == D3D11_USAGE_STAGING) && (desc.CPUAccessFlags & D3D11_CPU_ACCESS_READ) )
    {
        // Handle case where the source is already a staging texture we can use directly
        pStaging = pTexture;
    }
    else
    {
        // Otherwise, create a staging texture from the non-MSAA source
        desc.BindFlags = 0;
        desc.MiscFlags &= D3D11_RESOURCE_MISC_TEXTURECUBE;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        desc.Usage = D3D11_USAGE_STAGING;

        hr = d3dDevice->CreateTexture2D( &desc, 0, pStaging.GetAddressOf() );
        if ( FAILED(hr) )
            return hr;

        assert( pStaging );

        pContext->CopyResource( pStaging.Get(), pSource );
    }

    return S_OK;
}


//--------------------------------------------------------------------------------------
#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY != WINAPI_FAMILY_PHONE_APP)

static bool g_WIC2 = false;

static IWICImagingFactory* _GetWIC()
{
    static IWICImagingFactory* s_Factory = nullptr;

    if ( s_Factory )
        return s_Factory;

#if(_WIN32_WINNT >= _WIN32_WINNT_WIN8) || defined(_WIN7_PLATFORM_UPDATE)
    HRESULT hr = CoCreateInstance(
        CLSID_WICImagingFactory2,
        nullptr,
        CLSCTX_INPROC_SERVER,
        __uuidof(IWICImagingFactory2),
        (LPVOID*)&s_Factory
        );

    if ( SUCCEEDED(hr) )
    {
        // WIC2 is available on Windows 8 and Windows 7 SP1 with KB 2670838 installed
        g_WIC2 = true;
    }
    else
    {
        hr = CoCreateInstance(
            CLSID_WICImagingFactory1,
            nullptr,
            CLSCTX_INPROC_SERVER,
            __uuidof(IWICImagingFactory),
            (LPVOID*)&s_Factory
            );

        if ( FAILED(hr) )
        {
            s_Factory = nullptr;
            return nullptr;
        }
    }
#else
    HRESULT hr = CoCreateInstance(
        CLSID_WICImagingFactory,
        nullptr,
        CLSCTX_INPROC_SERVER,
        __uuidof(IWICImagingFactory),
        (LPVOID*)&s_Factory
        );

    if ( FAILED(hr) )
    {
        s_Factory = nullptr;
        return nullptr;
    }
#endif

    return s_Factory;
}
#endif


//--------------------------------------------------------------------------------------
HRESULT DirectX::SaveDDSTextureToFile( _In_ ID3D11DeviceContext* pContext,
                                       _In_ ID3D11Resource* pSource,
                                       _In_z_ LPCWSTR fileName )
{
    if ( !fileName )
        return E_INVALIDARG;

    D3D11_TEXTURE2D_DESC desc = { 0 };
    ComPtr<ID3D11Texture2D> pStaging;
    HRESULT hr = CaptureTexture( pContext, pSource, desc, pStaging );
    if ( FAILED(hr) )
        return hr;

    // Create file
#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)
    ScopedHandle hFile( safe_handle( CreateFile2( fileName, GENERIC_WRITE, 0, CREATE_ALWAYS, 0 ) ) );
#else
    ScopedHandle hFile( safe_handle( CreateFileW( fileName, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0 ) ) );
#endif
    if ( !hFile )
        return HRESULT_FROM_WIN32( GetLastError() );

    // Setup header
    const size_t MAX_HEADER_SIZE = sizeof(uint32_t) + sizeof(DDS_HEADER) + sizeof(DDS_HEADER_DXT10);
    uint8_t fileHeader[ MAX_HEADER_SIZE ];

    *reinterpret_cast<uint32_t*>(&fileHeader[0]) = DDS_MAGIC;

    auto header = reinterpret_cast<DDS_HEADER*>( reinterpret_cast<uint8_t*>(&fileHeader[0]) + sizeof(uint32_t) );
    size_t headerSize = sizeof(uint32_t) + sizeof(DDS_HEADER);
    memset( header, 0, sizeof(DDS_HEADER) );
    header->size = sizeof( DDS_HEADER );
    header->flags = DDS_HEADER_FLAGS_TEXTURE | DDS_HEADER_FLAGS_MIPMAP;
    header->height = desc.Height;
    header->width = desc.Width;
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
    case DXGI_FORMAT_B8G8R8A8_UNORM:        memcpy_s( &header->ddspf, sizeof(header->ddspf), &DDSPF_A8R8G8B8, sizeof(DDS_PIXELFORMAT) );    break; // DXGI 1.1
    case DXGI_FORMAT_B8G8R8X8_UNORM:        memcpy_s( &header->ddspf, sizeof(header->ddspf), &DDSPF_X8R8G8B8, sizeof(DDS_PIXELFORMAT) );    break; // DXGI 1.1
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

    default:
        memcpy_s( &header->ddspf, sizeof(header->ddspf), &DDSPF_DX10, sizeof(DDS_PIXELFORMAT) );

        headerSize += sizeof(DDS_HEADER_DXT10);
        extHeader = reinterpret_cast<DDS_HEADER_DXT10*>( reinterpret_cast<uint8_t*>(&fileHeader[0]) + sizeof(uint32_t) + sizeof(DDS_HEADER) );
        memset( extHeader, 0, sizeof(DDS_HEADER_DXT10) );
        extHeader->dxgiFormat = desc.Format;
        extHeader->resourceDimension = D3D11_RESOURCE_DIMENSION_TEXTURE2D;
        extHeader->arraySize = 1;
        break;
    }

    size_t rowPitch, slicePitch, rowCount;
    GetSurfaceInfo( desc.Width, desc.Height, desc.Format, &slicePitch, &rowPitch, &rowCount );

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

    D3D11_MAPPED_SUBRESOURCE mapped;
    hr = pContext->Map( pStaging.Get(), 0, D3D11_MAP_READ, 0, &mapped );
    if ( FAILED(hr) )
        return hr;

    auto sptr = reinterpret_cast<const uint8_t*>( mapped.pData );
    if ( !sptr )
    {
        pContext->Unmap( pStaging.Get(), 0 );
        return E_POINTER;
    }

    uint8_t* dptr = pixels.get();

    for( size_t h = 0; h < rowCount; ++h )
    {
        size_t msize = std::min<size_t>( rowPitch, mapped.RowPitch );
        memcpy_s( dptr, rowPitch, sptr, msize );
        sptr += mapped.RowPitch;
        dptr += rowPitch;
    }

    pContext->Unmap( pStaging.Get(), 0 );

    // Write header & pixels
    DWORD bytesWritten;
    if ( !WriteFile( hFile.get(), fileHeader, static_cast<DWORD>( headerSize ), &bytesWritten, 0 ) )
        return HRESULT_FROM_WIN32( GetLastError() );

    if ( bytesWritten != headerSize )
        return E_FAIL;

    if ( !WriteFile( hFile.get(), pixels.get(), static_cast<DWORD>( slicePitch ), &bytesWritten, 0 ) )
        return HRESULT_FROM_WIN32( GetLastError() );

    if ( bytesWritten != slicePitch )
        return E_FAIL;

    return S_OK;
}

//--------------------------------------------------------------------------------------
#if !defined(WINAPI_FAMILY) || (WINAPI_FAMILY != WINAPI_FAMILY_PHONE_APP)

HRESULT DirectX::SaveWICTextureToFile( _In_ ID3D11DeviceContext* pContext,
                                       _In_ ID3D11Resource* pSource,
                                       _In_ REFGUID guidContainerFormat, 
                                       _In_z_ LPCWSTR fileName,
                                       _In_opt_ const GUID* targetFormat,
                                       _In_opt_ std::function<void(IPropertyBag2*)> setCustomProps )
{
    if ( !fileName )
        return E_INVALIDARG;

    D3D11_TEXTURE2D_DESC desc = { 0 };
    ComPtr<ID3D11Texture2D> pStaging;
    HRESULT hr = CaptureTexture( pContext, pSource, desc, pStaging );
    if ( FAILED(hr) )
        return hr;

    // Determine source format's WIC equivalent
    WICPixelFormatGUID pfGuid;
    bool sRGB = false;
    switch ( desc.Format )
    {
    case DXGI_FORMAT_R32G32B32A32_FLOAT:            pfGuid = GUID_WICPixelFormat128bppRGBAFloat; break;
    case DXGI_FORMAT_R16G16B16A16_FLOAT:            pfGuid = GUID_WICPixelFormat64bppRGBAHalf; break;
    case DXGI_FORMAT_R16G16B16A16_UNORM:            pfGuid = GUID_WICPixelFormat64bppRGBA; break;
    case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:    pfGuid = GUID_WICPixelFormat32bppRGBA1010102XR; break; // DXGI 1.1
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

    case DXGI_FORMAT_B8G8R8A8_UNORM: // DXGI 1.1
        pfGuid = GUID_WICPixelFormat32bppBGRA;
        break;

    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB: // DXGI 1.1
        pfGuid = GUID_WICPixelFormat32bppBGRA;
        sRGB = true;
        break;

    case DXGI_FORMAT_B8G8R8X8_UNORM: // DXGI 1.1
        pfGuid = GUID_WICPixelFormat32bppBGR;
        break; 

    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB: // DXGI 1.1
        pfGuid = GUID_WICPixelFormat32bppBGR;
        sRGB = true;
        break; 

    default:
        return HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED );
    }

    IWICImagingFactory* pWIC = _GetWIC();
    if ( !pWIC )
        return E_NOINTERFACE;

    ComPtr<IWICStream> stream;
    hr = pWIC->CreateStream( stream.GetAddressOf() );
    if ( FAILED(hr) )
        return hr;

    hr = stream->InitializeFromFilename( fileName, GENERIC_WRITE );
    if ( FAILED(hr) )
        return hr;

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

    if ( targetFormat && memcmp( &guidContainerFormat, &GUID_ContainerFormatBmp, sizeof(WICPixelFormatGUID) ) == 0 && g_WIC2 )
    {
        // Opt-in to the WIC2 support for writing 32-bit Windows BMP files with an alpha channel
        PROPBAG2 option = { 0 };
        option.pstrName = L"EnableV5Header32bppBGRA";

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

    hr = frame->SetSize( desc.Width , desc.Height );
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
#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8) || defined(_WIN7_PLATFORM_UPDATE)
        case DXGI_FORMAT_R32G32B32A32_FLOAT:            
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
            if ( g_WIC2 )
            {
                targetGuid = GUID_WICPixelFormat96bppRGBFloat;
            }
            else
            {
                targetGuid = GUID_WICPixelFormat24bppBGR;
            }
            break;
#endif

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
        value.pszVal = "DirectXTK";

        if ( memcmp( &guidContainerFormat, &GUID_ContainerFormatPng, sizeof(GUID) ) == 0 )
        {
            // Set Software name
            (void)metawriter->SetMetadataByName( L"/tEXt/{str=Software}", &value );

            // Set sRGB chunk
            if ( sRGB )
            {
                value.vt = VT_UI1;
                value.bVal = 0;
                (void)metawriter->SetMetadataByName( L"/sRGB/RenderingIntent", &value );
            }
        }
        else
        {
            // Set Software name
            (void)metawriter->SetMetadataByName( L"System.ApplicationName", &value );

            if ( sRGB )
            {
                // Set JPEG EXIF Colorspace of sRGB
                value.vt = VT_UI2;
                value.uiVal = 1;
                (void)metawriter->SetMetadataByName( L"System.Image.ColorSpace", &value );
            }
        }
    }

    D3D11_MAPPED_SUBRESOURCE mapped;
    hr = pContext->Map( pStaging.Get(), 0, D3D11_MAP_READ, 0, &mapped );
    if ( FAILED(hr) )
        return hr;

    if ( memcmp( &targetGuid, &pfGuid, sizeof(WICPixelFormatGUID) ) != 0 )
    {
        // Conversion required to write
        ComPtr<IWICBitmap> source;
        hr = pWIC->CreateBitmapFromMemory( desc.Width, desc.Height, pfGuid,
                                           mapped.RowPitch, mapped.RowPitch * desc.Height,
                                           reinterpret_cast<BYTE*>( mapped.pData ), source.GetAddressOf() );
        if ( FAILED(hr) )
        {
            pContext->Unmap( pStaging.Get(), 0 );
            return hr;
        }

        ComPtr<IWICFormatConverter> FC;
        hr = pWIC->CreateFormatConverter( FC.GetAddressOf() );
        if ( FAILED(hr) )
        {
            pContext->Unmap( pStaging.Get(), 0 );
            return hr;
        }

        hr = FC->Initialize( source.Get(), targetGuid, WICBitmapDitherTypeNone, 0, 0, WICBitmapPaletteTypeCustom );
        if ( FAILED(hr) )
        {
            pContext->Unmap( pStaging.Get(), 0 );
            return hr;
        }

        WICRect rect = { 0, 0, (INT)desc.Width, (INT)desc.Height };
        hr = frame->WriteSource( FC.Get(), &rect );
        if ( FAILED(hr) )
        {
            pContext->Unmap( pStaging.Get(), 0 );
            return hr;
        }
    }
    else
    {
        // No conversion required
        hr = frame->WritePixels( desc.Height, mapped.RowPitch, mapped.RowPitch * desc.Height, reinterpret_cast<BYTE*>( mapped.pData ) );
        if ( FAILED(hr) )
            return hr;
    }

    pContext->Unmap( pStaging.Get(), 0 );

    hr = frame->Commit();
    if ( FAILED(hr) )
        return hr;

    hr = encoder->Commit();
    if ( FAILED(hr) )
        return hr;

    return S_OK;
}

#endif // !WINAPI_FAMILY || (WINAPI_FAMILY != WINAPI_FAMILY_PHONE_APP)
