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

	float dx = g_InputDxyz[addr + xOceanDtxAddressOffset].x * sign_correction * xOceanChoppyScale;
	float dy = g_InputDxyz[addr + xOceanDtyAddressOffset].x * sign_correction * xOceanChoppyScale;
	float dz = g_InputDxyz[addr].x * sign_correction;

	output[DTid.xy] = float4(dx, dy, dz, 1);
}
