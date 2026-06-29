#include "globals.hlsli"
#include "ShaderInterop_Ocean.h"

StructuredBuffer<float2> g_InputH0 : register(t0);
StructuredBuffer<float> g_InputOmega : register(t1);

RWStructuredBuffer<float2> g_OutputHt : register(u0);

// H(0) -> H(t)
[numthreads(OCEAN_COMPUTE_TILESIZE, OCEAN_COMPUTE_TILESIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	int in_index = DTid.y * xOceanInWidth + DTid.x;
	int in_mindex = (xOceanActualDim - DTid.y) * xOceanInWidth + (xOceanActualDim - DTid.x);
	int out_index = DTid.y * xOceanOutWidth + DTid.x;

	// H(0) -> H(t)
	float2 h0_k = g_InputH0[in_index];
	float2 h0_mk = g_InputH0[in_mindex];
	float sin_v, cos_v;
	sincos(g_InputOmega[in_index] * GetFrame().time * xOceanTimeScale, sin_v, cos_v);

	float2 ht;
	ht.x = (h0_k.x + h0_mk.x) * cos_v - (h0_k.y + h0_mk.y) * sin_v;
	ht.y = (h0_k.x - h0_mk.x) * sin_v + (h0_k.y - h0_mk.y) * cos_v;

	// H(t) -> Dx(t), Dy(t)
	float kx = DTid.x - xOceanActualDim * 0.5f;
	float ky = DTid.y - xOceanActualDim * 0.5f;
	float sqr_k = kx * kx + ky * ky;
	float rsqr_k = 0;
	if (sqr_k > 1e-12f)
		rsqr_k = 1 / sqrt(sqr_k);
	//float rsqr_k = 1 / sqrtf(kx * kx + ky * ky);
	kx *= rsqr_k;
	ky *= rsqr_k;
	float2 dt_x = float2(ht.y * kx, -ht.x * kx);
	float2 dt_y = float2(ht.y * ky, -ht.x * ky);

	if ((DTid.x < xOceanOutWidth) && (DTid.y < xOceanOutHeight))
	{
		// Two-for-one real FFT packing: the inverse transform of each field is
		// real (Hermitian input), so pack two fields into one complex slice.
		// Field 0 = ht + i*dt_x, so after the (linear) FFT the real output is
		// the height and the imaginary output is Dx. i*dt_x = (-dt_x.y, dt_x.x).
		g_OutputHt[out_index] = float2(ht.x - dt_x.y, ht.y + dt_x.x);
		// Field 1 carries Dy alone (recovered from its real output).
		g_OutputHt[out_index + xOceanSecondFieldOffset] = dt_y;
	}
}
