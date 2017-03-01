#include "objectHF.hlsli"

RWTEXTURE3D(output_emission, float4, 0);
RWTEXTURE3D(output_normal, float4, 1);

void main(float4 pos : SV_POSITION, float3 nor : NORMAL, float2 tex : TEXCOORD, float3 P : POSITION3D)
{
	float4 emission = DEGAMMA(xBaseColorMap.Sample(sampler_linear_clamp, tex));
	float4 normal = float4(nor * 0.5f + 0.5f, 1);

	float3 diff = (P - g_xWorld_VoxelRadianceDataCenter) * g_xWorld_VoxelRadianceRemap;
	float3 uvw = diff * float3(0.5f, -0.5f, 0.5f) + 0.5f;
	uint res = floor(g_xWorld_VoxelRadianceDataRes);
	uint3 writecoord = floor(uvw * res);

	if (writecoord.x >= 0 && writecoord.x < res
		&& writecoord.y >= 0 && writecoord.y < res
		&& writecoord.z >= 0 && writecoord.z < res)
	{
		output_emission[writecoord] = emission;
		output_normal[writecoord] = normal;
	}
}