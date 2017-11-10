#ifndef _OCEAN_SIMULATOR_HF_
#define _OCEAN_SIMULATOR_HF_

// Copyright (c) 2011 NVIDIA Corporation. All rights reserved.
//
// TO  THE MAXIMUM  EXTENT PERMITTED  BY APPLICABLE  LAW, THIS SOFTWARE  IS PROVIDED
// *AS IS*  AND NVIDIA AND  ITS SUPPLIERS DISCLAIM  ALL WARRANTIES,  EITHER  EXPRESS
// OR IMPLIED, INCLUDING, BUT NOT LIMITED  TO, NONINFRINGEMENT,IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL  NVIDIA 
// OR ITS SUPPLIERS BE  LIABLE  FOR  ANY  DIRECT, SPECIAL,  INCIDENTAL,  INDIRECT,  OR  
// CONSEQUENTIAL DAMAGES WHATSOEVER (INCLUDING, WITHOUT LIMITATION,  DAMAGES FOR LOSS 
// OF BUSINESS PROFITS, BUSINESS INTERRUPTION, LOSS OF BUSINESS INFORMATION, OR ANY 
// OTHER PECUNIARY LOSS) ARISING OUT OF THE  USE OF OR INABILITY  TO USE THIS SOFTWARE, 
// EVEN IF NVIDIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
//
// Please direct any bugs or questions to SDKFeedback@nvidia.com

//---------------------------------------- Vertex Shaders ------------------------------------------
struct VS_QUAD_OUTPUT
{
	float4 Position		: SV_POSITION;	// vertex position
	float2 TexCoord		: TEXCOORD0;	// vertex texture coords 
};

VS_QUAD_OUTPUT QuadVS(float4 vPos : POSITION)
{
	VS_QUAD_OUTPUT Output;

	Output.Position = vPos;
	Output.TexCoord.x = 0.5f + vPos.x * 0.5f;
	Output.TexCoord.y = 0.5f - vPos.y * 0.5f;

	return Output;
}

//----------------------------------------- Pixel Shaders ------------------------------------------

// Textures and sampling states
Texture2D g_samplerDisplacementMap : register(t0);

SamplerState LinearSampler : register(s0);

// Constants
cbuffer cbImmutable : register(b0)
{
	uint g_ActualDim;
	uint g_InWidth;
	uint g_OutWidth;
	uint g_OutHeight;
	uint g_DxAddressOffset;
	uint g_DyAddressOffset;
};

cbuffer cbChangePerFrame : register(b1)
{
	float g_Time;
	float g_ChoppyScale;
	float g_GridLen;
};

// The following three should contains only real numbers. But we have only C2C FFT now.
StructuredBuffer<float2>	g_InputDxyz		: register(t0);

#endif // _OCEAN_SIMULATOR_HF_
