#include "volumetricLightHF.hlsli"
#include "cone.hlsli"

VertexToPixel main(uint vid : SV_VERTEXID)
{
	VertexToPixel Out;

	float4 pos = float4(vertexID_create_cube(vid) * 2 - 1, 1);
	pos.z -= 1;
	pos.z *= 0.5;
	Out.pos = Out.pos2D = mul(g_xTransform, pos);
	return Out;
}
