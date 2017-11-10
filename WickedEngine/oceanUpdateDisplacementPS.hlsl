#include "oceanHF.hlsli"

// Post-FFT data wrap up: Dx, Dy, Dz -> Displacement
float4 main(VS_QUAD_OUTPUT In) : SV_Target
{
	uint index_x = (uint)(In.TexCoord.x * (float)g_OutWidth);
	uint index_y = (uint)(In.TexCoord.y * (float)g_OutHeight);
	uint addr = g_OutWidth * index_y + index_x;

	// cos(pi * (m1 + m2))
	int sign_correction = ((index_x + index_y) & 1) ? -1 : 1;

	float dx = g_InputDxyz[addr + g_DxAddressOffset].x * sign_correction * g_ChoppyScale;
	float dy = g_InputDxyz[addr + g_DyAddressOffset].x * sign_correction * g_ChoppyScale;
	float dz = g_InputDxyz[addr].x * sign_correction;

	return float4(dx, dy, dz, 1);
}
