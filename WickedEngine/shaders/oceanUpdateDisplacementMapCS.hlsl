#include "globals.hlsli"
#include "ShaderInterop_Ocean.h"

StructuredBuffer<float2> g_InputDxyz : register(t0);

RWTexture2D<float4> output : register(u0);

[numthreads(OCEAN_COMPUTE_TILESIZE, OCEAN_COMPUTE_TILESIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	uint addr = xOceanOutWidth * DTid.y + DTid.x;

	// cos(pi * (m1 + m2))
	int sign_correction = ((DTid.x + DTid.y) & 1) ? -1 : 1;

	// Field 0 packs height in its real output and Dx in its imaginary output;
	// field 1 carries Dy in its real output (see oceanSimulatorCS two-for-one).
	float2 f0 = g_InputDxyz[addr];
	float f1 = g_InputDxyz[addr + xOceanSecondFieldOffset].x;

	float dz = f0.x * sign_correction;
	float dx = f0.y * sign_correction * xOceanChoppyScale;
	float dy = f1 * sign_correction * xOceanChoppyScale;

	output[DTid.xy] = float4(dx, dy, dz, 1);
}
