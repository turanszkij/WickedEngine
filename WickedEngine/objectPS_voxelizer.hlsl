#include "objectHF.hlsli"
#include "voxelHF.hlsli"

RWSTRUCTUREDBUFFER(output, VoxelType, 0);

void main(float4 pos : SV_POSITION, float3 N : NORMAL, float2 tex : TEXCOORD, float3 P : POSITION3D, nointerpolation float3 instanceColor : COLOR)
{
	float3 diff = (P - g_xFrame_VoxelRadianceDataCenter) * g_xFrame_VoxelRadianceDataRes_Inverse * g_xFrame_VoxelRadianceDataSize_Inverse;
	float3 uvw = diff * float3(0.5f, -0.5f, 0.5f) + 0.5f;

	[branch]
	if (!any(uvw - saturate(uvw)))
	{
		float4 baseColor = DEGAMMA(g_xMat_baseColor * float4(instanceColor, 1) * xBaseColorMap.Sample(sampler_linear_wrap, tex));
		float4 color = baseColor;
		float emissive = g_xMat_emissive;

		// fake normals are good enough because it's only coarse diffuse light, no need to normalize:
		//	(just uncomment if there are any noticable artifacts)
		//N = normalize(N);

		float3 diffuse = 0;

		for (uint i = g_xFrame_LightArrayOffset; i < g_xFrame_LightArrayCount; ++i)
		{
			ShaderEntityType light = EntityArray[i];

			if (light.GetFlags() & ENTITY_FLAG_LIGHT_STATIC)
			{
				continue; // static lights will be skipped (they are used in lightmap baking)
			}

			LightingResult result = (LightingResult)0;

			switch (light.GetType())
			{
			case ENTITY_TYPE_DIRECTIONALLIGHT:
			{
				float3 L = light.directionWS;

				float3 lightColor = light.GetColor().rgb * light.energy * max(dot(N, L), 0);

				[branch]
				if (light.additionalData_index >= 0)
				{
					float4 ShPos = mul(float4(P, 1), MatrixArray[light.additionalData_index + 0]);
					ShPos.xyz /= ShPos.w;
					float3 ShTex = ShPos.xyz * float3(0.5f, -0.5f, 0.5f) + 0.5f;

					[branch]if ((saturate(ShTex.x) == ShTex.x) && (saturate(ShTex.y) == ShTex.y) && (saturate(ShTex.z) == ShTex.z))
					{
						lightColor *= shadowCascade(ShPos, ShTex.xy, light.shadowKernel, light.shadowBias, light.additionalData_index + 0);
					}
				}

				result.diffuse = lightColor;
			}
			break;
			case ENTITY_TYPE_POINTLIGHT:
			{
				float3 L = light.positionWS - P;
				const float dist2 = dot(L, L);
				const float dist = sqrt(dist2);

				[branch]
				if (dist < light.range)
				{
					L /= dist;

					const float range2 = light.range * light.range;
					const float att = saturate(1.0 - (dist2 / range2));
					const float attenuation = att * att;

					float3 lightColor = light.GetColor().rgb * light.energy * max(dot(N, L), 0) * attenuation;

					[branch]
					if (light.additionalData_index >= 0) {
						lightColor *= texture_shadowarray_cube.SampleCmpLevelZero(sampler_cmp_depth, float4(-L, light.additionalData_index), 1 - dist / light.range * (1 - light.shadowBias)).r;
					}

					result.diffuse = lightColor;
				}
			}
			break;
			case ENTITY_TYPE_SPOTLIGHT:
			{
				float3 L = light.positionWS - P;
				const float dist2 = dot(L, L);
				const float dist = sqrt(dist2);

				[branch]
				if (dist < light.range)
				{
					L /= dist;

					const float SpotFactor = dot(L, light.directionWS);
					const float spotCutOff = light.coneAngleCos;

					[branch]
					if (SpotFactor > spotCutOff)
					{
						const float range2 = light.range * light.range;
						const float att = saturate(1.0 - (dist2 / range2));
						float attenuation = att * att;
						attenuation *= saturate((1.0 - (1.0 - SpotFactor) * 1.0 / (1.0 - spotCutOff)));

						float3 lightColor = light.GetColor().rgb * light.energy * max(dot(N, L), 0) * attenuation;

						[branch]
						if (light.additionalData_index >= 0)
						{
							float4 ShPos = mul(float4(P, 1), MatrixArray[light.additionalData_index + 0]);
							ShPos.xyz /= ShPos.w;
							float2 ShTex = ShPos.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);
							[branch]
							if ((saturate(ShTex.x) == ShTex.x) && (saturate(ShTex.y) == ShTex.y))
							{
								lightColor *= shadowCascade(ShPos, ShTex.xy, light.shadowKernel, light.shadowBias, light.additionalData_index);
							}
						}

						result.diffuse = lightColor;
					}
				}
			}
			break;
			}

			diffuse += result.diffuse;
		}

		color.rgb *= diffuse;
		
		color.rgb += baseColor.rgb * emissive;

		uint color_encoded = EncodeColor(color);
		uint normal_encoded = EncodeNormal(N);

		// output:
		uint3 writecoord = floor(uvw * g_xFrame_VoxelRadianceDataRes);
		uint id = flatten3D(writecoord, g_xFrame_VoxelRadianceDataRes);
		InterlockedMax(output[id].colorMask, color_encoded);
		InterlockedMax(output[id].normalMask, normal_encoded);
	}
}