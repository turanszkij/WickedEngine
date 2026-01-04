//-------------------------------------------------------------------------------------------------------------------------------------------------------------
//
// Copyright 2023-2025 Apple Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
//-------------------------------------------------------------------------------------------------------------------------------------------------------------

#ifndef IRROOTSIGNATUREFLAGS_H
#define IRROOTSIGNATUREFLAGS_H

typedef enum IRRootSignatureFlags
{
    IRRootSignatureFlagNone                              = 0,
    IRRootSignatureFlagAllowInputAssemblerInputLayout    = 0x1,
    IRRootSignatureFlagDenyVertexShaderRootAccess        = 0x2,
    IRRootSignatureFlagDenyHullShaderRootAccess          = 0x4,
    IRRootSignatureFlagDenyDomainShaderRootAccess        = 0x8,
    IRRootSignatureFlagDenyGeometryShaderRootAccess      = 0x10,
    IRRootSignatureFlagDenyPixelShaderRootAccess         = 0x20,
    IRRootSignatureFlagAllowStreamOutput                 = 0x40,
    IRRootSignatureFlagLocalRootSignature                = 0x80,
    IRRootSignatureFlagDenyAmplificationShaderRootAccess = 0x100,
    IRRootSignatureFlagDenyMeshShaderRootAccess          = 0x200,
    IRRootSignatureFlagCBVSRVUAVHeapDirectlyIndexed      = 0x400,
    IRRootSignatureFlagSamplerHeapDirectlyIndexed        = 0x800,
} IRRootSignatureFlags;

#endif // IRROOTSIGNATUREFLAGS_H
