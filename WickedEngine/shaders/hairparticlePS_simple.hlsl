#include "globals.hlsli"
#include "hairparticleHF.hlsli"

float4 main(VertexToPixel input) : SV_TARGET
{
	return g_xMaterial.baseColor;
}
