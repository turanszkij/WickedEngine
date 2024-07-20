#include "globals.hlsli"
#include "hairparticleHF.hlsli"

float4 main(VertexToPixel input) : SV_TARGET
{
	ShaderMaterial material = HairGetMaterial();

	return material.GetBaseColor();
}
