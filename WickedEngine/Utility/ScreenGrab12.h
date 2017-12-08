//--------------------------------------------------------------------------------------
// File: ScreenGrab12.h
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

#pragma once

#include <d3d12.h>

#include <ocidl.h>
#include <stdint.h>
#include <functional>


namespace DirectX
{
    HRESULT __cdecl SaveDDSTextureToFile(
        _In_ ID3D12CommandQueue* pCommandQueue,
        _In_ ID3D12Resource* pSource,
        _In_z_ const wchar_t* fileName,
        D3D12_RESOURCE_STATES beforeState = D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATES afterState = D3D12_RESOURCE_STATE_RENDER_TARGET);

    HRESULT __cdecl SaveWICTextureToFile(
        _In_ ID3D12CommandQueue* pCommandQ,
        _In_ ID3D12Resource* pSource,
        REFGUID guidContainerFormat,
        _In_z_ const wchar_t* fileName,
        D3D12_RESOURCE_STATES beforeState = D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATES afterState = D3D12_RESOURCE_STATE_RENDER_TARGET,
        _In_opt_ const GUID* targetFormat = nullptr,
        _In_opt_ std::function<void __cdecl(IPropertyBag2*)> setCustomProps = nullptr);
}