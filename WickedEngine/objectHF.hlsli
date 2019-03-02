#ifndef _OBJECTSHADER_HF_
#define _OBJECTSHADER_HF_

#if defined(TILEDFORWARD) && !defined(TRANSPARENT)
#define DISABLE_ALPHATEST
#endif

#ifdef TRANSPARENT
#define DISABLE_TRANSPARENT_SHADOWMAP
#endif


#ifdef PLANARREFLECTION
#define DISABLE_ENVMAPS
#endif


#define LIGHTMAP_QUALITY_BICUBIC


#include "globals.hlsli"
#include "objectInputLayoutHF.hlsli"
#include "ditherHF.hlsli"
#include "tangentComputeHF.hlsli"
#include "depthConvertHF.hlsli"
#include "fogHF.hlsli"
#include "brdf.hlsli"
#include "packHF.hlsli"
#include "lightingHF.hlsli"

// DEFINITIONS
//////////////////

// These are bound by wiRenderer (based on Material):
#define xBaseColorMap			texture_0	// rgb: baseColor, a: opacity
#define xNormalMap				texture_1	// rgb: normal
#define xSurfaceMap				texture_2	// r: ao, g: roughness, b: metallic, a: reflectance
#define xDisplacementMap		texture_3	// r: heightmap
#define xEmissiveMap			texture_4	// rgba: emissive

// These are bound by RenderPath (based on Render Path):
#define xReflection				texture_6	// rgba: scene color from reflected camera angle
#define xRefraction				texture_7	// rgba: scene color from primary camera angle
#define	xWaterRipples			texture_8	// rgb: snorm8 water ripple normal map
#define	xSSAO					texture_8	// r: screen space ambient occlusion
#define	xSSR					texture_9	// rgb: screen space ray-traced reflections, a: reflection blend based on ray hit or miss


struct PixelInputType_Simple
{
	float4 pos								: SV_POSITION;
	float  clip								: SV_ClipDistance0;
	float2 tex								: TEXCOORD0;
	nointerpolation float  dither			: DITHER;
	nointerpolation float3 instanceColor	: INSTANCECOLOR;
};
struct PixelInputType
{
	float4 pos								: SV_POSITION;
	float  clip								: SV_ClipDistance0;
	float2 tex								: TEXCOORD0;
	nointerpolation float  dither			: DITHER;
	nointerpolation float3 instanceColor	: INSTANCECOLOR;
	float3 nor								: NORMAL;
	float4 pos2D							: SCREENPOSITION;
	float3 pos3D							: WORLDPOSITION;
	float4 pos2DPrev						: SCREENPOSITIONPREV;
	float4 ReflectionMapSamplingPos			: TEXCOORD1;
	float2 nor2D							: NORMAL2D;
	float2 atl								: ATLAS;
};

struct GBUFFEROutputType
{
	float4 g0	: SV_Target0;		// texture_gbuffer0
	float4 g1	: SV_Target1;		// texture_gbuffer1
	float4 g2	: SV_Target2;		// texture_gbuffer2
	float4 diffuse	: SV_Target3;
	float4 specular	: SV_Target4;
};
inline GBUFFEROutputType CreateGbuffer(in float4 color, in Surface surface, in float2 velocity, in float3 diffuse, in float3 specular)
{
	GBUFFEROutputType Out;
	Out.g0 = float4(color.rgb, surface.sss);													/*FORMAT_R8G8B8A8_UNORM*/
	Out.g1 = float4(encode(surface.N), velocity);												/*FORMAT_R16G16B16A16_FLOAT*/
	Out.g2 = float4(surface.ao, surface.roughness, surface.metalness, surface.reflectance);		/*FORMAT_R8G8B8A8_UNORM*/
	Out.diffuse = float4(diffuse, 1);															/*wiRenderer::RTFormat_deferred_lightbuffer*/
	Out.specular = float4(specular, 1);															/*wiRenderer::RTFormat_deferred_lightbuffer*/
	return Out;
}

struct GBUFFEROutputType_Thin
{
	float4 g0	: SV_Target0;		// texture_gbuffer0
	float4 g1	: SV_Target1;		// texture_gbuffer1
};
inline GBUFFEROutputType_Thin CreateGbuffer_Thin(in float4 color, in Surface surface, in float2 velocity)
{
	GBUFFEROutputType_Thin Out;
	Out.g0 = color;																		/*FORMAT_R16G16B16A16_FLOAT*/
	Out.g1 = float4(encode(surface.N), velocity);										/*FORMAT_R16G16B16A16_FLOAT*/
	return Out;
}


// METHODS
////////////

inline void ApplyEmissive(in Surface surface, inout float3 specular)
{
	specular += surface.emissiveColor.rgb * surface.emissiveColor.a;
}

inline void LightMapping(in float2 ATLAS, inout float3 diffuse, inout float3 specular, inout float ao, in float ssao)
{
	if (any(ATLAS))
	{
#ifdef LIGHTMAP_QUALITY_BICUBIC
		float4 lightmap = SampleTextureCatmullRom(texture_globallightmap, ATLAS);
#else
		float4 lightmap = texture_globallightmap.SampleLevel(sampler_linear_clamp, ATLAS, 0);
#endif // LIGHTMAP_QUALITY_BICUBIC
		diffuse += lightmap.rgb * ssao;
		ao *= saturate(1 - lightmap.a);
	}
}

inline void NormalMapping(in float2 UV, in float3 V, inout float3 N, in float3x3 TBN, inout float3 bumpColor)
{
	float3 normalMap = xNormalMap.Sample(sampler_objectshader, UV).rgb;
	bumpColor = normalMap.rgb * 2 - 1;
	bumpColor.g *= g_xMat_normalMapFlip;
	N = normalize(lerp(N, mul(bumpColor, TBN), g_xMat_normalMapStrength));
	bumpColor *= g_xMat_normalMapStrength;
}

inline void SpecularAA(in float3 N, inout float roughness)
{
	[branch]
	if (g_xFrame_SpecularAA > 0)
	{
		float3 ddxN = ddx_coarse(N);
		float3 ddyN = ddy_coarse(N);
		float curve = pow(max(dot(ddxN, ddxN), dot(ddyN, ddyN)), 1 - g_xFrame_SpecularAA);
		roughness = max(roughness, curve);
	}
}

inline float3 PlanarReflection(in float2 reflectionUV, in float2 bumpColor)
{
	return xReflection.SampleLevel(sampler_linear_clamp, reflectionUV + bumpColor*g_xMat_normalMapStrength, 0).rgb;
}

#define NUM_PARALLAX_OCCLUSION_STEPS 32
inline void ParallaxOcclusionMapping(inout float2 UV, in float3 V, in float3x3 TBN)
{
	V = mul(TBN, V);
	float layerHeight = 1.0 / NUM_PARALLAX_OCCLUSION_STEPS;
	float curLayerHeight = 0;
	float2 dtex = g_xMat_parallaxOcclusionMapping * V.xy / NUM_PARALLAX_OCCLUSION_STEPS;
	float2 currentTextureCoords = UV;
	float2 derivX = ddx_coarse(UV);
	float2 derivY = ddy_coarse(UV);
	float heightFromTexture = 1 - xDisplacementMap.SampleGrad(sampler_linear_wrap, currentTextureCoords, derivX, derivY).r;
	uint iter = 0;
	[loop]
	while (heightFromTexture > curLayerHeight && iter < NUM_PARALLAX_OCCLUSION_STEPS)
	{
		curLayerHeight += layerHeight;
		currentTextureCoords -= dtex;
		heightFromTexture = 1 - xDisplacementMap.SampleGrad(sampler_linear_wrap, currentTextureCoords, derivX, derivY).r;
		iter++;
	}
	float2 prevTCoords = currentTextureCoords + dtex;
	float nextH = heightFromTexture - curLayerHeight;
	float prevH = 1 - xDisplacementMap.SampleGrad(sampler_linear_wrap, prevTCoords, derivX, derivY).r - curLayerHeight + layerHeight;
	float weight = nextH / (nextH - prevH);
	float2 finalTexCoords = prevTCoords * weight + currentTextureCoords * (1.0 - weight);
	UV = finalTexCoords;
}

inline void Refraction(in float2 ScreenCoord, in float2 normal2D, in float3 bumpColor, inout Surface surface, inout float4 color, inout float3 diffuse)
{
	if (g_xMat_refractionIndex > 0)
	{
		float2 size;
		float mipLevels;
		xRefraction.GetDimensions(0, size.x, size.y, mipLevels);
		const float sampled_mip = (g_xFrame_AdvancedRefractions ? surface.roughness * mipLevels : 0);
		float2 perturbatedRefrTexCoords = ScreenCoord.xy + (normal2D + bumpColor.rg) * g_xMat_refractionIndex;
		float4 refractiveColor = xRefraction.SampleLevel(sampler_linear_clamp, perturbatedRefrTexCoords, sampled_mip);
		surface.albedo.rgb = lerp(refractiveColor.rgb, surface.albedo.rgb, color.a);
		diffuse = lerp(1, diffuse, color.a);
		color.a = 1;
	}
}

inline void ForwardLighting(inout Surface surface, inout float3 diffuse, inout float3 specular, inout float3 reflection)
{
#ifndef DISABLE_DECALS
	[branch]
	if (xForwardDecalMask != 0)
	{
		// decals are enabled, loop through them first:
		float4 decalAccumulation = 0;
		const float3 P_dx = ddx_coarse(surface.P);
		const float3 P_dy = ddy_coarse(surface.P);

		uint bucket_bits = xForwardDecalMask;

		[loop]
		while (bucket_bits != 0)
		{
			// Retrieve global entity index from local bucket, then remove bit from local bucket:
			const uint bucket_bit_index = firstbitlow(bucket_bits);
			const uint entity_index = bucket_bit_index;
			bucket_bits ^= 1 << bucket_bit_index;

			[branch]
			if (decalAccumulation.a < 1)
			{
				ShaderEntityType decal = EntityArray[g_xFrame_DecalArrayOffset + entity_index];

				const float4x4 decalProjection = MatrixArray[decal.userdata];
				const float3 clipSpacePos = mul(float4(surface.P, 1), decalProjection).xyz;
				const float3 uvw = clipSpacePos.xyz*float3(0.5f, -0.5f, 0.5f) + 0.5f;
				[branch]
				if (is_saturated(uvw))
				{
					// mipmapping needs to be performed by hand:
					const float2 decalDX = mul(P_dx, (float3x3)decalProjection).xy * decal.texMulAdd.xy;
					const float2 decalDY = mul(P_dy, (float3x3)decalProjection).xy * decal.texMulAdd.xy;
					float4 decalColor = texture_decalatlas.SampleGrad(sampler_linear_clamp, uvw.xy*decal.texMulAdd.xy + decal.texMulAdd.zw, decalDX, decalDY);
					// blend out if close to cube Z:
					float edgeBlend = 1 - pow(saturate(abs(clipSpacePos.z)), 8);
					decalColor.a *= edgeBlend;
					decalColor *= decal.GetColor();
					// apply emissive:
					specular += max(0, decalColor.rgb * decal.GetEmissive() * edgeBlend);
					// perform manual blending of decals:
					//  NOTE: they are sorted top-to-bottom, but blending is performed bottom-to-top
					decalAccumulation.rgb = (1 - decalAccumulation.a) * (decalColor.a*decalColor.rgb) + decalAccumulation.rgb;
					decalAccumulation.a = decalColor.a + (1 - decalColor.a) * decalAccumulation.a;
					[branch]
					if (decalAccumulation.a >= 1.0)
					{
						// force exit:
						break;
					}
				}
			}
			else
			{
				// force exit:
				break;
			}

		}

		surface.albedo.rgb = lerp(surface.albedo.rgb, decalAccumulation.rgb, decalAccumulation.a);
	}
#endif // DISABLE_DECALS


//#ifndef DISABLE_ENVMAPS
//	const float envMapMIP = surface.roughness * g_xFrame_EnvProbeMipCount;
//	reflection = max(0, EnvironmentReflection_Global(surface, envMapMIP));
//#endif

#ifndef DISABLE_ENVMAPS
	// Apply environment maps:
	float4 envmapAccumulation = 0;
	const float envMapMIP = surface.roughness * g_xFrame_EnvProbeMipCount;

#ifndef ENVMAPRENDERING
#ifndef DISABLE_LOCALENVPMAPS
	[branch]
	if (xForwardEnvProbeMask != 0)
	{
		uint bucket_bits = xForwardEnvProbeMask;

		[loop]
		while (bucket_bits != 0)
		{
			// Retrieve global entity index from local bucket, then remove bit from local bucket:
			const uint bucket_bit_index = firstbitlow(bucket_bits);
			const uint entity_index = bucket_bit_index;
			bucket_bits ^= 1 << bucket_bit_index;

			[branch]
			if (envmapAccumulation.a < 1)
			{
				ShaderEntityType probe = EntityArray[g_xFrame_EnvProbeArrayOffset + entity_index];

				const float4x4 probeProjection = MatrixArray[probe.userdata];
				const float3 clipSpacePos = mul(float4(surface.P, 1), probeProjection).xyz;
				const float3 uvw = clipSpacePos.xyz*float3(0.5f, -0.5f, 0.5f) + 0.5f;
				[branch]
				if (is_saturated(uvw))
				{
					const float4 envmapColor = EnvironmentReflection_Local(surface, probe, probeProjection, clipSpacePos, envMapMIP);
					// perform manual blending of probes:
					//  NOTE: they are sorted top-to-bottom, but blending is performed bottom-to-top
					envmapAccumulation.rgb = (1 - envmapAccumulation.a) * (envmapColor.a * envmapColor.rgb) + envmapAccumulation.rgb;
					envmapAccumulation.a = envmapColor.a + (1 - envmapColor.a) * envmapAccumulation.a;
					[branch]
					if (envmapAccumulation.a >= 1.0)
					{
						// force exit:
						break;
					}
				}
			}
			else
			{
				// force exit:
				break;
			}

		}
	}
#endif // DISABLE_LOCALENVPMAPS
#endif //ENVMAPRENDERING

	// Apply global envmap where there is no local envmap information:
	[branch]
	if (envmapAccumulation.a < 0.99f)
	{
		envmapAccumulation.rgb = lerp(EnvironmentReflection_Global(surface, envMapMIP), envmapAccumulation.rgb, envmapAccumulation.a);
	}
	reflection = max(0, envmapAccumulation.rgb);
#endif // DISABLE_ENVMAPS

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

				ShaderEntityType light = EntityArray[g_xFrame_LightArrayOffset + entity_index];

				if (light.GetFlags() & ENTITY_FLAG_LIGHT_STATIC)
				{
					continue; // static lights will be skipped (they are used in lightmap baking)
				}

				LightingResult result = (LightingResult)0;

				switch (light.GetType())
				{
				case ENTITY_TYPE_DIRECTIONALLIGHT:
				{
					result = DirectionalLight(light, surface);
				}
				break;
				case ENTITY_TYPE_POINTLIGHT:
				{
					result = PointLight(light, surface);
				}
				break;
				case ENTITY_TYPE_SPOTLIGHT:
				{
					result = SpotLight(light, surface);
				}
				break;
				case ENTITY_TYPE_SPHERELIGHT:
				{
					result = SphereLight(light, surface);
				}
				break;
				case ENTITY_TYPE_DISCLIGHT:
				{
					result = DiscLight(light, surface);
				}
				break;
				case ENTITY_TYPE_RECTANGLELIGHT:
				{
					result = RectangleLight(light, surface);
				}
				break;
				case ENTITY_TYPE_TUBELIGHT:
				{
					result = TubeLight(light, surface);
				}
				break;
				}

				diffuse += max(0.0f, result.diffuse);
				specular += max(0.0f, result.specular);
			}
		}
	}

}

inline void TiledLighting(in float2 pixel, inout Surface surface, inout float3 diffuse, inout float3 specular, inout float3 reflection)
{
	const uint2 tileIndex = uint2(floor(pixel / TILED_CULLING_BLOCKSIZE));
	const uint flatTileIndex = flatten2D(tileIndex, g_xFrame_EntityCullingTileCount.xy) * SHADER_ENTITY_TILE_BUCKET_COUNT;


#ifndef DISABLE_DECALS
	[branch]
	if (g_xFrame_DecalArrayCount > 0)
	{
		// decals are enabled, loop through them first:
		float4 decalAccumulation = 0;
		const float3 P_dx = ddx_coarse(surface.P);
		const float3 P_dy = ddy_coarse(surface.P);

		// Loop through decal buckets in the tile:
		const uint first_item = g_xFrame_DecalArrayOffset;
		const uint last_item = first_item + g_xFrame_DecalArrayCount - 1;
		const uint first_bucket = first_item / 32;
		const uint last_bucket = min(last_item / 32, max(0, SHADER_ENTITY_TILE_BUCKET_COUNT - 1));
		[loop]
		for (uint bucket = first_bucket; bucket <= last_bucket; ++bucket)
		{
			uint bucket_bits = EntityTiles[flatTileIndex + bucket];

			// This is the wave scalarizer from Improved Culling - Siggraph 2017 [Drobot]:
			bucket_bits = WaveReadLaneFirst(WaveActiveBitOr(bucket_bits));

			[loop]
			while (bucket_bits != 0)
			{
				// Retrieve global entity index from local bucket, then remove bit from local bucket:
				const uint bucket_bit_index = firstbitlow(bucket_bits);
				const uint entity_index = bucket * 32 + bucket_bit_index;
				bucket_bits ^= 1 << bucket_bit_index;

				[branch]
				if (entity_index >= first_item && entity_index <= last_item && decalAccumulation.a < 1)
				{
					ShaderEntityType decal = EntityArray[entity_index];

					const float4x4 decalProjection = MatrixArray[decal.userdata];
					const float3 clipSpacePos = mul(float4(surface.P, 1), decalProjection).xyz;
					const float3 uvw = clipSpacePos.xyz*float3(0.5f, -0.5f, 0.5f) + 0.5f;
					[branch]
					if (is_saturated(uvw))
					{
						// mipmapping needs to be performed by hand:
						const float2 decalDX = mul(P_dx, (float3x3)decalProjection).xy * decal.texMulAdd.xy;
						const float2 decalDY = mul(P_dy, (float3x3)decalProjection).xy * decal.texMulAdd.xy;
						float4 decalColor = texture_decalatlas.SampleGrad(sampler_linear_clamp, uvw.xy*decal.texMulAdd.xy + decal.texMulAdd.zw, decalDX, decalDY);
						// blend out if close to cube Z:
						float edgeBlend = 1 - pow(saturate(abs(clipSpacePos.z)), 8);
						decalColor.a *= edgeBlend;
						decalColor *= decal.GetColor();
						// apply emissive:
						specular += max(0, decalColor.rgb * decal.GetEmissive() * edgeBlend);
						// perform manual blending of decals:
						//  NOTE: they are sorted top-to-bottom, but blending is performed bottom-to-top
						decalAccumulation.rgb = (1 - decalAccumulation.a) * (decalColor.a*decalColor.rgb) + decalAccumulation.rgb;
						decalAccumulation.a = decalColor.a + (1 - decalColor.a) * decalAccumulation.a;
						[branch]
						if (decalAccumulation.a >= 1.0)
						{
							// force exit:
							bucket = SHADER_ENTITY_TILE_BUCKET_COUNT;
							break;
						}
					}
				}
				else if (entity_index > last_item)
				{
					// force exit:
					bucket = SHADER_ENTITY_TILE_BUCKET_COUNT;
					break;
				}

			}
		}

		surface.albedo.rgb = lerp(surface.albedo.rgb, decalAccumulation.rgb, decalAccumulation.a);
	}
#endif // DISABLE_DECALS


#ifndef DISABLE_ENVMAPS
	// Apply environment maps:
	float4 envmapAccumulation = 0;
	const float envMapMIP = surface.roughness * g_xFrame_EnvProbeMipCount;

#ifndef DISABLE_LOCALENVPMAPS
	[branch]
	if (g_xFrame_EnvProbeArrayCount > 0)
	{
		// Loop through envmap buckets in the tile:
		const uint first_item = g_xFrame_EnvProbeArrayOffset;
		const uint last_item = first_item + g_xFrame_EnvProbeArrayCount - 1;
		const uint first_bucket = first_item / 32;
		const uint last_bucket = min(last_item / 32, max(0, SHADER_ENTITY_TILE_BUCKET_COUNT - 1));
		[loop]
		for (uint bucket = first_bucket; bucket <= last_bucket; ++bucket)
		{
			uint bucket_bits = EntityTiles[flatTileIndex + bucket];

			// Bucket scalarizer - Siggraph 2017 - Improved Culling [Michal Drobot]:
			bucket_bits = WaveReadLaneFirst(WaveActiveBitOr(bucket_bits));

			[loop]
			while (bucket_bits != 0)
			{
				// Retrieve global entity index from local bucket, then remove bit from local bucket:
				const uint bucket_bit_index = firstbitlow(bucket_bits);
				const uint entity_index = bucket * 32 + bucket_bit_index;
				bucket_bits ^= 1 << bucket_bit_index;

				[branch]
				if (entity_index >= first_item && entity_index <= last_item && envmapAccumulation.a < 1)
				{
					ShaderEntityType probe = EntityArray[entity_index];

					const float4x4 probeProjection = MatrixArray[probe.userdata];
					const float3 clipSpacePos = mul(float4(surface.P, 1), probeProjection).xyz;
					const float3 uvw = clipSpacePos.xyz*float3(0.5f, -0.5f, 0.5f) + 0.5f;
					[branch]
					if (is_saturated(uvw))
					{
						const float4 envmapColor = EnvironmentReflection_Local(surface, probe, probeProjection, clipSpacePos, envMapMIP);
						// perform manual blending of probes:
						//  NOTE: they are sorted top-to-bottom, but blending is performed bottom-to-top
						envmapAccumulation.rgb = (1 - envmapAccumulation.a) * (envmapColor.a * envmapColor.rgb) + envmapAccumulation.rgb;
						envmapAccumulation.a = envmapColor.a + (1 - envmapColor.a) * envmapAccumulation.a;
						[branch]
						if (envmapAccumulation.a >= 1.0)
						{
							// force exit:
							bucket = SHADER_ENTITY_TILE_BUCKET_COUNT;
							break;
						}
					}
				}
				else if (entity_index > last_item)
				{
					// force exit:
					bucket = SHADER_ENTITY_TILE_BUCKET_COUNT;
					break;
				}

			}
		}
	}
#endif // DISABLE_LOCALENVPMAPS
	
	// Apply global envmap where there is no local envmap information:
	[branch]
	if (envmapAccumulation.a < 0.99f)
	{
		envmapAccumulation.rgb = lerp(EnvironmentReflection_Global(surface, envMapMIP), envmapAccumulation.rgb, envmapAccumulation.a);
	}
	reflection = max(0, envmapAccumulation.rgb);
#endif // DISABLE_ENVMAPS


	[branch]
	if (g_xFrame_LightArrayCount > 0)
	{
		// Loop through light buckets in the tile:
		const uint first_item = g_xFrame_LightArrayOffset;
		const uint last_item = first_item + g_xFrame_LightArrayCount - 1;
		const uint first_bucket = first_item / 32;
		const uint last_bucket = min(last_item / 32, max(0, SHADER_ENTITY_TILE_BUCKET_COUNT - 1));
		[loop]
		for (uint bucket = first_bucket; bucket <= last_bucket; ++bucket)
		{
			uint bucket_bits = EntityTiles[flatTileIndex + bucket];

			// Bucket scalarizer - Siggraph 2017 - Improved Culling [Michal Drobot]:
			bucket_bits = WaveReadLaneFirst(WaveActiveBitOr(bucket_bits));

			[loop]
			while (bucket_bits != 0)
			{
				// Retrieve global entity index from local bucket, then remove bit from local bucket:
				const uint bucket_bit_index = firstbitlow(bucket_bits);
				const uint entity_index = bucket * 32 + bucket_bit_index;
				bucket_bits ^= 1 << bucket_bit_index;

				// Check if it is a light and process:
				[branch]
				if (entity_index >= first_item && entity_index <= last_item)
				{
					ShaderEntityType light = EntityArray[entity_index];

					if (light.GetFlags() & ENTITY_FLAG_LIGHT_STATIC)
					{
						continue; // static lights will be skipped (they are used in lightmap baking)
					}

					LightingResult result = (LightingResult)0;

					switch (light.GetType())
					{
					case ENTITY_TYPE_DIRECTIONALLIGHT:
					{
						result = DirectionalLight(light, surface);
					}
					break;
					case ENTITY_TYPE_POINTLIGHT:
					{
						result = PointLight(light, surface);
					}
					break;
					case ENTITY_TYPE_SPOTLIGHT:
					{
						result = SpotLight(light, surface);
					}
					break;
					case ENTITY_TYPE_SPHERELIGHT:
					{
						result = SphereLight(light, surface);
					}
					break;
					case ENTITY_TYPE_DISCLIGHT:
					{
						result = DiscLight(light, surface);
					}
					break;
					case ENTITY_TYPE_RECTANGLELIGHT:
					{
						result = RectangleLight(light, surface);
					}
					break;
					case ENTITY_TYPE_TUBELIGHT:
					{
						result = TubeLight(light, surface);
					}
					break;
					}

					diffuse += max(0.0f, result.diffuse);
					specular += max(0.0f, result.specular);
				}
				else if (entity_index > last_item)
				{
					// force exit:
					bucket = SHADER_ENTITY_TILE_BUCKET_COUNT;
					break;
				}

			}
		}
	}

}

inline void ApplyLighting(in Surface surface, in float3 diffuse, in float3 specular, inout float4 color)
{
	color.rgb = (GetAmbient(surface.N) * surface.ao + diffuse) * surface.albedo + specular;
}

inline void ApplyFog(in float dist, inout float4 color)
{
	color.rgb = lerp(color.rgb, GetHorizonColor(), GetFog(dist));
}


// OBJECT SHADER PROTOTYPE
///////////////////////////

#if defined(COMPILE_OBJECTSHADER_PS)

// Possible switches:
//	ALPHATESTONLY		-	assemble object shader for depth only rendering + alpha test
//	TEXTUREONLY			-	assemble object shader for rendering only with base textures, no lighting
//	DEFERRED			-	assemble object shader for deferred rendering
//	FORWARD				-	assemble object shader for forward rendering
//	TILEDFORWARD		-	assemble object shader for tiled forward rendering
//	TRANSPARENT			-	assemble object shader for forward or tile forward transparent rendering
//	ENVMAPRENDERING		-	modify object shader for envmap rendering
//	NORMALMAP			-	include normal mapping computation
//	PLANARREFLECTION	-	include planar reflection sampling
//	POM					-	include parallax occlusion mapping computation
//	WATER				-	include specialized water shader code
//	BLACKOUT			-	include specialized blackout shader code

#if defined(ALPHATESTONLY) || defined(TEXTUREONLY)
#define SIMPLE_INPUT
#endif // APLHATESTONLY

#ifdef SIMPLE_INPUT
#define PIXELINPUT PixelInputType_Simple
#else
#define PIXELINPUT PixelInputType
#endif // SIMPLE_INPUT


// entry point:
#if defined(ALPHATESTONLY)
void main(PIXELINPUT input)
#elif defined(TEXTUREONLY)
float4 main(PIXELINPUT input) : SV_TARGET
#elif defined(TRANSPARENT)
float4 main(PIXELINPUT input) : SV_TARGET
#elif defined(ENVMAPRENDERING)
float4 main(PSIn_EnvmapRendering input) : SV_TARGET
#elif defined(DEFERRED)
GBUFFEROutputType main(PIXELINPUT input)
#elif defined(FORWARD)
GBUFFEROutputType_Thin main(PIXELINPUT input)
#elif defined(TILEDFORWARD)
[earlydepthstencil]
GBUFFEROutputType_Thin main(PIXELINPUT input)
#endif // ALPHATESTONLY



// shader base:
{
	float2 pixel = input.pos.xy;

#if !(defined(TILEDFORWARD) && !defined(TRANSPARENT)) && !defined(ENVMAPRENDERING)
	// apply dithering:
	clip(dither(pixel + GetTemporalAASampleRotation()) - input.dither);
#endif



	float2 UV = input.tex * g_xMat_texMulAdd.xy + g_xMat_texMulAdd.zw;

	Surface surface;

#ifndef SIMPLE_INPUT
	surface.P = input.pos3D;
	surface.V = g_xCamera_CamPos - surface.P;
	float dist = length(surface.V);
	surface.V /= dist;
	surface.N = normalize(input.nor);

	float3x3 TBN = compute_tangent_frame(surface.N, surface.P, UV);
#endif // SIMPLE_INPUT

#ifdef POM
	ParallaxOcclusionMapping(UV, surface.V, TBN);
#endif // POM

	float4 color = xBaseColorMap.Sample(sampler_objectshader, UV);
	color.rgb = DEGAMMA(color.rgb);
	color *= g_xMat_baseColor * float4(input.instanceColor, 1);
	ALPHATEST(color.a);

#ifndef SIMPLE_INPUT
	float3 diffuse = 0;
	float3 specular = 0;
	float3 reflection = 0;
	float3 bumpColor = 0;
	float opacity = color.a;
	float depth = input.pos.z;
	float ssao = 1;
#ifndef ENVMAPRENDERING
	const float lineardepth = input.pos2D.w;
	input.pos2D.xy /= input.pos2D.w;
	input.pos2DPrev.xy /= input.pos2DPrev.w;
	input.ReflectionMapSamplingPos.xy /= input.ReflectionMapSamplingPos.w;

	const float2 refUV = input.ReflectionMapSamplingPos.xy * float2(0.5f, -0.5f) + 0.5f;
	const float2 ScreenCoord = input.pos2D.xy * float2(0.5f, -0.5f) + 0.5f;
	const float2 velocity = ((input.pos2DPrev.xy - g_xFrame_TemporalAAJitterPrev) - (input.pos2D.xy - g_xFrame_TemporalAAJitter)) * float2(0.5f, -0.5f);
	const float2 ReprojectedScreenCoord = ScreenCoord + velocity;

	const float sampled_lineardepth = texture_lineardepth.SampleLevel(sampler_point_clamp, ScreenCoord, 0) * g_xFrame_MainCamera_ZFarP;
	const float depth_difference = sampled_lineardepth - lineardepth;
#endif // ENVMAPRENDERING
#endif // SIMPLE_INPUT

#ifdef NORMALMAP
	NormalMapping(UV, surface.P, surface.N, TBN, bumpColor);
#endif // NORMALMAP

	float4 surface_ao_roughness_metallic_reflectance = xSurfaceMap.Sample(sampler_objectshader, UV);
	float4 emissiveColor;
	[branch]
	if (g_xMat_emissiveColor.a > 0)
	{
		emissiveColor = xEmissiveMap.Sample(sampler_objectshader, UV);
		emissiveColor.rgb = DEGAMMA(emissiveColor.rgb);
		emissiveColor *= g_xMat_emissiveColor;
	}
	else 
	{
		emissiveColor = 0;
	}

	surface = CreateSurface(
		surface.P, 
		surface.N, 
		surface.V, 
		color,
		surface_ao_roughness_metallic_reflectance.r,
		g_xMat_roughness * surface_ao_roughness_metallic_reflectance.g,
		g_xMat_metalness * surface_ao_roughness_metallic_reflectance.b,
		g_xMat_reflectance * surface_ao_roughness_metallic_reflectance.a,
		emissiveColor,
		g_xMat_subsurfaceScattering
	);


#ifndef SIMPLE_INPUT


#ifdef WATER
	color.a = 1;

	//NORMALMAP
	float2 bumpColor0 = 0;
	float2 bumpColor1 = 0;
	float2 bumpColor2 = 0;
	bumpColor0 = 2.0f * xNormalMap.Sample(sampler_objectshader, UV - g_xMat_texMulAdd.ww).rg - 1.0f;
	bumpColor1 = 2.0f * xNormalMap.Sample(sampler_objectshader, UV + g_xMat_texMulAdd.zw).rg - 1.0f;
	bumpColor2 = xWaterRipples.Sample(sampler_objectshader, ScreenCoord).rg;
	bumpColor = float3(bumpColor0 + bumpColor1 + bumpColor2, 1)  * g_xMat_refractionIndex;
	surface.N = normalize(lerp(surface.N, mul(normalize(bumpColor), TBN), g_xMat_normalMapStrength));
	bumpColor *= g_xMat_normalMapStrength;

	//REFLECTION
	float4 reflectiveColor = xReflection.SampleLevel(sampler_linear_mirror, refUV + bumpColor.rg, 0);


	//REFRACTION 
	float2 perturbatedRefrTexCoords = ScreenCoord.xy + bumpColor.rg;
	float3 refractiveColor = xRefraction.SampleLevel(sampler_linear_mirror, perturbatedRefrTexCoords, 0).rgb;
	float mod = saturate(0.05f * depth_difference);
	refractiveColor = lerp(refractiveColor, surface.baseColor.rgb, mod).rgb;

	//FRESNEL TERM
	float3 fresnelTerm = F_Fresnel(surface.f0, surface.NdotV);
	surface.albedo.rgb = lerp(refractiveColor, reflectiveColor.rgb, fresnelTerm);
#endif // WATER


#ifndef ENVMAPRENDERING
#ifndef TRANSPARENT
	ssao = xSSAO.SampleLevel(sampler_linear_clamp, ReprojectedScreenCoord, 0).r;
	surface.ao *= ssao;
#endif // TRANSPARENT
#endif // ENVMAPRENDERING



	SpecularAA(surface.N, surface.roughness);

	ApplyEmissive(surface, specular);

	LightMapping(input.atl, diffuse, specular, surface.ao, ssao);



#ifdef DEFERRED


#ifdef PLANARREFLECTION
	specular += PlanarReflection(refUV, bumpColor.rg) * surface.F;
#endif



#else // not DEFERRED

#ifdef FORWARD
	ForwardLighting(surface, diffuse, specular, reflection);
#endif // FORWARD

#ifdef TILEDFORWARD
	TiledLighting(pixel, surface, diffuse, specular, reflection);
#endif // TILEDFORWARD


#ifndef WATER
#ifndef ENVMAPRENDERING

	VoxelGI(surface, diffuse, reflection);

#ifdef PLANARREFLECTION
	reflection = PlanarReflection(refUV, bumpColor.rg);
#endif


#ifdef TRANSPARENT
	Refraction(ScreenCoord, input.nor2D, bumpColor, surface, color, diffuse);
#else
	float4 ssr = xSSR.SampleLevel(sampler_linear_clamp, ReprojectedScreenCoord, 0);
	reflection = lerp(reflection, ssr.rgb, ssr.a);
#endif // TRANSPARENT


#endif // ENVMAPRENDERING
#endif // WATER

	specular += reflection * surface.F;

	ApplyLighting(surface, diffuse, specular, color);

#ifdef WATER
	// SOFT EDGE
	float fade = saturate(0.3 * depth_difference);
	color.a *= fade;
#endif // WATER

	ApplyFog(dist, color);


#endif // DEFERRED


#ifdef TEXTUREONLY
	color.rgb += surface.emissiveColor.rgb * surface.emissiveColor.a;
#endif // TEXTUREONLY


#ifdef BLACKOUT
	color = float4(0, 0, 0, 1);
#endif

#endif // SIMPLE_INPUT

	color = max(0, color);


	// end point:
#if defined(TRANSPARENT) || defined(TEXTUREONLY) || defined(ENVMAPRENDERING)
	return color;
#else
#if defined(DEFERRED)	
	return CreateGbuffer(color, surface, velocity, diffuse, specular);
#elif defined(FORWARD) || defined(TILEDFORWARD)
	return CreateGbuffer_Thin(color, surface, velocity);
#endif // DEFERRED
#endif // TRANSPARENT

}


#endif // COMPILE_OBJECTSHADER_PS



#endif // _OBJECTSHADER_HF_

