#include "objectHF.hlsli"
#include "voxelHF.hlsli"

RWSTRUCTUREDBUFFER(output, VoxelType, 0);

void main(float4 pos : SV_POSITION, float3 N : NORMAL, float2 tex : TEXCOORD, float3 P : POSITION3D, float3 instanceColor : COLOR)
{
	float3 diff = (P - g_xWorld_VoxelRadianceDataCenter) / g_xWorld_VoxelRadianceDataRes / g_xWorld_VoxelRadianceDataSize;
	float3 uvw = diff * float3(0.5f, -0.5f, 0.5f) + 0.5f;
	uint3 writecoord = floor(uvw * g_xWorld_VoxelRadianceDataRes);

	[branch]
	if (writecoord.x > 0 && writecoord.x < g_xWorld_VoxelRadianceDataRes
		&& writecoord.y > 0 && writecoord.y < g_xWorld_VoxelRadianceDataRes
		&& writecoord.z > 0 && writecoord.z < g_xWorld_VoxelRadianceDataRes)
	{
		float4 baseColor = DEGAMMA(g_xMat_baseColor * float4(instanceColor, 1) * xBaseColorMap.Sample(sampler_linear_wrap, tex));
		float4 color = baseColor;
		float emissive = g_xMat_emissive;

		[branch]
		if (g_xFrame_SunLightArrayIndex >= 0)
		{
			LightArrayType light = LightArray[g_xFrame_SunLightArrayIndex];

			float3 L = light.directionWS;

			float3 diffuse = light.color.rgb * light.energy * max(dot(N, L), 0);

			[branch]
			if (light.shadowMap_index >= 0)
			{
				float4 ShPos = mul(float4(P, 1), light.shadowMat[0]);
				ShPos.xyz /= ShPos.w;
				float3 ShTex = ShPos.xyz*float3(1, -1, 1) / 2.0f + 0.5f;

				[branch]if ((saturate(ShTex.x) == ShTex.x) && (saturate(ShTex.y) == ShTex.y) && (saturate(ShTex.z) == ShTex.z))
				{
					diffuse *= shadowCascade(ShPos, ShTex.xy, light.shadowKernel, light.shadowBias, light.shadowMap_index + 0);
				}
			}

			color.rgb *= diffuse;
		}
		
		OBJECT_PS_EMISSIVE

		uint color_encoded = EncodeColor(color);
		uint normal_encoded = EncodeNormal(N);

		// output:
		uint id = to1D(writecoord, g_xWorld_VoxelRadianceDataRes);
		InterlockedMax(output[id].colorMask, color_encoded);
		//InterlockedMax(output[id].normalMask, normal_encoded);
	}
}