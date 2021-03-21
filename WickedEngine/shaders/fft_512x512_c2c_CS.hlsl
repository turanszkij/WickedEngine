#include "ShaderInterop_FFTGenerator.h"

#define COS_PI_4_16 0.70710678118654752440084436210485f
#define TWIDDLE_1_8 COS_PI_4_16, -COS_PI_4_16
#define TWIDDLE_3_8 -COS_PI_4_16, -COS_PI_4_16
 
void FT2(inout float2 a, inout float2 b)
{
	float t;

	t = a.x;
	a.x += b.x;
	b.x = t - b.x;

	t = a.y;
	a.y += b.y;
	b.y = t - b.y;
}

void CMUL_forward(inout float2 a, float bx, float by)
{
	float t = a.x;
	a.x = t * bx - a.y * by;
	a.y = t * by + a.y * bx;
}

void UPD_forward(inout float2 a, inout float2 b)
{
	float A = a.x;
	float B = b.y;

	a.x += b.y;
	b.y = a.y + b.x;
	a.y -= b.x;
	b.x = A - B;
}

void FFT_forward_4(inout float2 D[8])
{
	FT2(D[0], D[2]);
	FT2(D[1], D[3]);
	FT2(D[0], D[1]);

	UPD_forward(D[2], D[3]);
}

void FFT_forward_8(inout float2 D[8])
{
	FT2(D[0], D[4]);
	FT2(D[1], D[5]);
	FT2(D[2], D[6]);
	FT2(D[3], D[7]);

	UPD_forward(D[4], D[6]);
	UPD_forward(D[5], D[7]);

	CMUL_forward(D[5], TWIDDLE_1_8);
	CMUL_forward(D[7], TWIDDLE_3_8);

	FFT_forward_4(D);
	FT2(D[4], D[5]);
	FT2(D[6], D[7]);
}

void TWIDDLE(inout float2 d, float phase)
{
	float tx, ty;

	sincos(phase, ty, tx);
	float t = d.x;
	d.x = t * tx - d.y * ty;
	d.y = t * ty + d.y * tx;
}

void TWIDDLE_8(inout float2 D[8], float phase)
{
	TWIDDLE(D[4], 1 * phase);
	TWIDDLE(D[2], 2 * phase);
	TWIDDLE(D[6], 3 * phase);
	TWIDDLE(D[1], 4 * phase);
	TWIDDLE(D[5], 5 * phase);
	TWIDDLE(D[3], 6 * phase);
	TWIDDLE(D[7], 7 * phase);
}

STRUCTUREDBUFFER(g_SrcData, float2, TEXSLOT_ONDEMAND0);
RWSTRUCTUREDBUFFER(g_DstData, float2, 0);

#ifndef FFT_V2

[numthreads(COHERENCY_GRANULARITY, 1, 1)]
void main(uint3 thread_id : SV_DispatchThreadID)
{
	if (thread_id.x >= thread_count)
		return;

	// Fetch 8 complex numbers
	float2 D[8];

	uint i;
	uint imod = thread_id.x & (istride - 1);
	uint iaddr = ((thread_id.x - imod) << 3) + imod;
	for (i = 0; i < 8; i++)
		D[i] = g_SrcData[iaddr + i * istride];

	// Math
	FFT_forward_8(D);
	uint p = thread_id.x & (istride - pstride);
	float phase = phase_base * (float)p;
	TWIDDLE_8(D, phase);

	// Store the result
	uint omod = thread_id.x & (ostride - 1);
	uint oaddr = ((thread_id.x - omod) << 3) + omod;
	g_DstData[oaddr + 0 * ostride] = D[0];
	g_DstData[oaddr + 1 * ostride] = D[4];
	g_DstData[oaddr + 2 * ostride] = D[2];
	g_DstData[oaddr + 3 * ostride] = D[6];
	g_DstData[oaddr + 4 * ostride] = D[1];
	g_DstData[oaddr + 5 * ostride] = D[5];
	g_DstData[oaddr + 6 * ostride] = D[3];
	g_DstData[oaddr + 7 * ostride] = D[7];
}

#else

[numthreads(COHERENCY_GRANULARITY, 1, 1)]
void main(uint3 thread_id : SV_DispatchThreadID)
{
	if (thread_id.x >= thread_count)
		return;

	// Fetch 8 complex numbers
	uint i;
	float2 D[8];
	uint iaddr = thread_id.x << 3;
	for (i = 0; i < 8; i++)
		D[i] = g_SrcData[iaddr + i];

	// Math
	FFT_forward_8(D);

	// Store the result
	uint omod = thread_id.x & (ostride - 1);
	uint oaddr = ((thread_id.x - omod) << 3) + omod;
	g_DstData[oaddr + 0 * ostride] = D[0];
	g_DstData[oaddr + 1 * ostride] = D[4];
	g_DstData[oaddr + 2 * ostride] = D[2];
	g_DstData[oaddr + 3 * ostride] = D[6];
	g_DstData[oaddr + 4 * ostride] = D[1];
	g_DstData[oaddr + 5 * ostride] = D[5];
	g_DstData[oaddr + 6 * ostride] = D[3];
	g_DstData[oaddr + 7 * ostride] = D[7];
}

#endif // FFT_V2
