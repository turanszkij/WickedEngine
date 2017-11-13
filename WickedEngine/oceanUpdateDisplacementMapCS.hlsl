#include "ShaderInterop_Ocean.h"

STRUCTUREDBUFFER(g_InputDxyz, float2, TEXSLOT_ONDEMAND0);
RWTEXTURE2D(output, float4, 0);

[numthreads(OCEAN_COMPUTE_TILESIZE, OCEAN_COMPUTE_TILESIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	uint addr = g_OutWidth * DTid.y + DTid.x;

	// cos(pi * (m1 + m2))
	int sign_correction = ((DTid.x + DTid.y) & 1) ? -1 : 1;

	float dx = g_InputDxyz[addr + g_DtxAddressOffset].x * sign_correction * g_ChoppyScale;
	float dy = g_InputDxyz[addr + g_DtyAddressOffset].x * sign_correction * g_ChoppyScale;
	float dz = g_InputDxyz[addr].x * sign_correction;

	output[DTid.xy] = float4(dx, dy, dz, 1);
}
