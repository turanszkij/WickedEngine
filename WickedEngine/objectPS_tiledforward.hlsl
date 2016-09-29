#include "objectHF.hlsli"
#include "lightCullingCSInterop.h"
#include "tiledLightingHF.hlsli"

TEXTURE2D(LightGrid, uint2, TEXSLOT_LIGHTGRID);
STRUCTUREDBUFFER(LightIndexList, uint, SBSLOT_LIGHTINDEXLIST);
STRUCTUREDBUFFER(LightArray, LightArrayType, SBSLOT_LIGHTARRAY);

struct LightingResult
{
	float3 diffuse;
	float3 specular;
};

float4 main(PixelInputType input) : SV_TARGET
{
	OBJECT_PS_DITHER

	OBJECT_PS_MAKE

	OBJECT_PS_SAMPLETEXTURES

	OBJECT_PS_DEGAMMA

	OBJECT_PS_LIGHT_BEGIN

	// TILED LIGHTING
	{
		uint2 tileIndex = uint2(floor(input.pos.xy / BLOCK_SIZE));
		uint startOffset = LightGrid[tileIndex].x;
		uint lightCount = LightGrid[tileIndex].y;

		specular = 0;
		diffuse = 0;
		for (uint i = 0; i < lightCount; i++)
		{
			uint lightIndex = LightIndexList[startOffset + i];
			LightArrayType light = LightArray[lightIndex];

			float3 L = light.PositionWS - P;
			float lightDistance = length(L);
			if (lightDistance > light.range)
				continue;
			L /= lightDistance;

			LightingResult result = (LightingResult)0;

			switch (light.type)
			{
			case 0/*DIRECTIONAL*/:
			{
			}
			break;
			case 1/*POINT*/:
			{
				BRDF_MAKE(N, L, V);
				result.specular = light.color.rgb * BRDF_SPECULAR(roughness, f0);
				result.diffuse = light.color.rgb * BRDF_DIFFUSE(roughness);
				result.diffuse *= light.energy;
				result.specular *= light.energy;

				float att = (light.energy * (light.range / (light.range + 1 + lightDistance)));
				float attenuation = /*saturate*/(att * (light.range - lightDistance) / light.range);
				result.diffuse *= attenuation;
				result.specular *= attenuation;

				float sh = max(NdotL, 0);
				//[branch]if (xLightEnerDis.w) {
				//	const float3 lv = P - xLightPos.xyz;
				//	static const float bias = 0.025;
				//	sh *= texture_shadow_cube.SampleCmpLevelZero(sampler_cmp_depth, lv, length(lv) / xLightEnerDis.y - bias).r;
				//}
				result.diffuse *= sh;
				result.specular *= sh;

				result.diffuse = max(result.diffuse, 0);
				result.specular = max(result.specular, 0);
			}
			break;
			case 2/*SPOT*/:
			{
			}
			break;
			}

			diffuse += result.diffuse;
			specular += result.specular;
		}

	}

	OBJECT_PS_ENVIRONMENTREFLECTIONS

	OBJECT_PS_LIGHT_END

	OBJECT_PS_EMISSIVE

	OBJECT_PS_FOG

	OBJECT_PS_OUT_FORWARD
}
