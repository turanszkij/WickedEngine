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

#define PI 3.1415926536f
#define BLOCK_SIZE_X 16
#define BLOCK_SIZE_Y 16

cbuffer cbImmutable : register(b0)
{
	uint g_ActualDim;
	uint g_InWidth;
	uint g_OutWidth;
	uint g_OutHeight;
	uint g_DtxAddressOffset;
	uint g_DtyAddressOffset;
};

cbuffer cbChangePerFrame : register(b1)
{
	float g_Time;
	float g_ChoppyScale;
};

StructuredBuffer<float2>	g_InputH0		: register(t0);
StructuredBuffer<float>		g_InputOmega	: register(t1);
RWStructuredBuffer<float2>	g_OutputHt		: register(u0);


//---------------------------------------- Compute Shaders -----------------------------------------

// Pre-FFT data preparation:

// Notice: In CS5.0, we can output up to 8 RWBuffers but in CS4.x only one output buffer is allowed,
// that way we have to allocate one big buffer and manage the offsets manually. The restriction is
// not caused by NVIDIA GPUs and does not present on NVIDIA GPUs when using other computing APIs like
// CUDA and OpenCL.

// H(0) -> H(t)
[numthreads(BLOCK_SIZE_X, BLOCK_SIZE_Y, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	int in_index = DTid.y * g_InWidth + DTid.x;
	int in_mindex = (g_ActualDim - DTid.y) * g_InWidth + (g_ActualDim - DTid.x);
	int out_index = DTid.y * g_OutWidth + DTid.x;

	// H(0) -> H(t)
	float2 h0_k = g_InputH0[in_index];
	float2 h0_mk = g_InputH0[in_mindex];
	float sin_v, cos_v;
	sincos(g_InputOmega[in_index] * g_Time, sin_v, cos_v);

	float2 ht;
	ht.x = (h0_k.x + h0_mk.x) * cos_v - (h0_k.y + h0_mk.y) * sin_v;
	ht.y = (h0_k.x - h0_mk.x) * sin_v + (h0_k.y - h0_mk.y) * cos_v;

	// H(t) -> Dx(t), Dy(t)
	float kx = DTid.x - g_ActualDim * 0.5f;
	float ky = DTid.y - g_ActualDim * 0.5f;
	float sqr_k = kx * kx + ky * ky;
	float rsqr_k = 0;
	if (sqr_k > 1e-12f)
		rsqr_k = 1 / sqrt(sqr_k);
	//float rsqr_k = 1 / sqrtf(kx * kx + ky * ky);
	kx *= rsqr_k;
	ky *= rsqr_k;
	float2 dt_x = float2(ht.y * kx, -ht.x * kx);
	float2 dt_y = float2(ht.y * ky, -ht.x * ky);

	if ((DTid.x < g_OutWidth) && (DTid.y < g_OutHeight))
	{
		g_OutputHt[out_index] = ht;
		g_OutputHt[out_index + g_DtxAddressOffset] = dt_x;
		g_OutputHt[out_index + g_DtyAddressOffset] = dt_y;
	}
}
