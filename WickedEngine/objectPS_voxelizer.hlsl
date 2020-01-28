#define DISABLE_SOFT_SHADOWS
#include "objectHF.hlsli"
#include "voxelHF.hlsli"

RWSTRUCTUREDBUFFER(output, VoxelType, 0);

struct PSInput
{
	float4 pos : SV_POSITION;
	float4 color : COLOR;
	float4 uvsets : UVSETS;
	float3 N : NORMAL;
	float3 P : POSITION3D;
};

void main(PSInput input)
{
	float3 N = input.N;
	float3 P = input.P;

	float3 diff = (P - g_xFrame_VoxelRadianceDataCenter) * g_xFrame_VoxelRadianceDataRes_rcp * g_xFrame_VoxelRadianceDataSize_rcp;
	float3 uvw = diff * float3(0.5f, -0.5f, 0.5f) + 0.5f;

	[branch]
	if (is_saturated(uvw))
	{
		float4 baseColor;
		[branch]
		if (g_xMaterial.uvset_baseColorMap >= 0)
		{
			const float2 UV_baseColorMap = g_xMaterial.uvset_baseColorMap == 0 ? input.uvsets.xy : input.uvsets.zw;
			baseColor = texture_basecolormap.Sample(sampler_linear_wrap, UV_baseColorMap);
			baseColor.rgb = DEGAMMA(baseColor.rgb);
		}
		else
		{
			baseColor = 1;
		}
		baseColor *= input.color;
		float4 color = baseColor;
		float4 emissiveColor = g_xMaterial.emissiveColor;
		[branch]
		if (g_xMaterial.emissiveColor.a > 0 && g_xMaterial.uvset_emissiveMap >= 0)
		{
			const float2 UV_emissiveMap = g_xMaterial.uvset_emissiveMap == 0 ? input.uvsets.xy : input.uvsets.zw;
			float4 emissiveMap = texture_emissivemap.Sample(sampler_linear_wrap, UV_emissiveMap);
			emissiveMap.rgb = DEGAMMA(emissiveMap.rgb);
			emissiveColor *= emissiveMap;
		}

		N = normalize(N);

		Lighting lighting = CreateLighting(0, 0, 0, 0);

		[branch]
		if (any(xForwardLightMask))
		{
			// Loop through light buckets for the draw call:
			const uint first_item = 0;
			const uint last_item = first_item + g_xFrame_LightArrayCount - 1;
			const uint first_bucket = first_item / 32;
			const uint last_bucket = min(last_item / 32, 1); // only 2 buckets max (uint2) for forward pass!
			[loop]
			for (uint bucket = first_bucket; bucket <= last_bucket; ++bucket)
			{
				uint bucket_bits = xForwardLightMask[bucket];

				[loop]
				while (bucket_bits != 0)
				{
					// Retrieve global entity index from local bucket, then remove bit from local bucket:
					const uint bucket_bit_index = firstbitlow(bucket_bits);
					const uint entity_index = bucket * 32 + bucket_bit_index;
					bucket_bits ^= 1 << bucket_bit_index;

					ShaderEntity light = EntityArray[g_xFrame_LightArrayOffset + entity_index];

					if (light.GetFlags() & ENTITY_FLAG_LIGHT_STATIC)
					{
						continue; // static lights will be skipped (they are used in lightmap baking)
					}

					switch (light.GetType())
					{
					case ENTITY_TYPE_DIRECTIONALLIGHT:
					{
						float3 L = light.directionWS;
						const float NdotL = saturate(dot(L, N));

						[branch]
						if (NdotL > 0)
						{
							float3 lightColor = light.GetColor().rgb * light.energy * NdotL;

							[branch]
							if (light.IsCastingShadow() >= 0)
							{
								const uint cascade = g_xFrame_ShadowCascadeCount - 1; // biggest cascade (coarsest resolution) will be used to voxelize
								float3 ShPos = mul(MatrixArray[light.GetShadowMatrixIndex() + cascade], float4(P, 1)).xyz; // ortho matrix, no divide by .w
								float3 ShTex = ShPos.xyz * float3(0.5f, -0.5f, 0.5f) + 0.5f;

								[branch] if ((saturate(ShTex.x) == ShTex.x) && (saturate(ShTex.y) == ShTex.y) && (saturate(ShTex.z) == ShTex.z))
								{
									lightColor *= shadowCascade(light, ShPos, ShTex.xy, cascade);
								}
							}

							lighting.direct.diffuse += lightColor;
						}
					}
					break;
					case ENTITY_TYPE_POINTLIGHT:
					{
						float3 L = light.positionWS - P;
						const float dist2 = dot(L, L);
						const float range2 = light.range * light.range;

						[branch]
						if (dist2 < range2)
						{
							const float3 Lunnormalized = L;
							const float dist = sqrt(dist2);
							L /= dist;
							const float NdotL = saturate(dot(L, N));

							[branch]
							if (NdotL > 0)
							{
								const float att = saturate(1.0 - (dist2 / range2));
								const float attenuation = att * att;

								float3 lightColor = light.GetColor().rgb * light.energy * NdotL * attenuation;

								[branch]
								if (light.IsCastingShadow() >= 0) {
									lightColor *= shadowCube(light, Lunnormalized);
								}

								lighting.direct.diffuse += lightColor;
							}
						}
					}
					break;
					case ENTITY_TYPE_SPOTLIGHT:
					{
						float3 L = light.positionWS - P;
						const float dist2 = dot(L, L);
						const float range2 = light.range * light.range;

						[branch]
						if (dist2 < range2)
						{
							const float dist = sqrt(dist2);
							L /= dist;
							const float NdotL = saturate(dot(L, N));

							[branch]
							if (NdotL > 0)
							{
								const float SpotFactor = dot(L, light.directionWS);
								const float spotCutOff = light.coneAngleCos;

								[branch]
								if (SpotFactor > spotCutOff)
								{
									const float range2 = light.range * light.range;
									const float att = saturate(1.0 - (dist2 / range2));
									float attenuation = att * att;
									attenuation *= saturate((1.0 - (1.0 - SpotFactor) * 1.0 / (1.0 - spotCutOff)));

									float3 lightColor = light.GetColor().rgb * light.energy * NdotL * attenuation;

									[branch]
									if (light.IsCastingShadow() >= 0)
									{
										float4 ShPos = mul(MatrixArray[light.GetShadowMatrixIndex() + 0], float4(P, 1));
										ShPos.xyz /= ShPos.w;
										float2 ShTex = ShPos.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);
										[branch]
										if ((saturate(ShTex.x) == ShTex.x) && (saturate(ShTex.y) == ShTex.y))
										{
											lightColor *= shadowCascade(light, ShPos.xyz, ShTex.xy, 0);
										}
									}

									lighting.direct.diffuse += lightColor;
								}
							}
						}
					}
					break;
					}
				}
			}
		}

		color.rgb *= lighting.direct.diffuse;
		
		color.rgb += emissiveColor.rgb * emissiveColor.a;

		uint color_encoded = EncodeColor(color);
		uint normal_encoded = EncodeNormal(N);

		// output:
		uint3 writecoord = floor(uvw * g_xFrame_VoxelRadianceDataRes);
		uint id = flatten3D(writecoord, g_xFrame_VoxelRadianceDataRes);
		InterlockedMax(output[id].colorMask, color_encoded);
		InterlockedMax(output[id].normalMask, normal_encoded);
	}
}
