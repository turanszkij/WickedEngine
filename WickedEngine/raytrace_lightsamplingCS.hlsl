//#define RAY_BACKFACE_CULLING
#include "globals.hlsli"
#include "ShaderInterop_TracedRendering.h"
#include "ShaderInterop_BVH.h"
#include "tracedRenderingHF.hlsli"
#include "raySceneIntersectHF.hlsli"

RAWBUFFER(counterBuffer_READ, TEXSLOT_ONDEMAND7);
STRUCTUREDBUFFER(rayIndexBuffer_READ, uint, TEXSLOT_ONDEMAND8);
STRUCTUREDBUFFER(rayBuffer_READ, TracedRenderingStoredRay, TEXSLOT_ONDEMAND9);

RWTEXTURE2D(resultTexture, float4, 0);

[numthreads(TRACEDRENDERING_TRACE_GROUPSIZE, 1, 1)]
void main( uint3 DTid : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex)
{
	// Initialize ray and pixel ID as non-contributing:
	Ray ray = (Ray)0;
	uint pixelID = 0xFFFFFFFF;

	if (DTid.x < counterBuffer_READ.Load(0))
	{
		// Load the current ray:
		LoadRay(rayBuffer_READ[rayIndexBuffer_READ[DTid.x]], ray, pixelID);

		// Compute real pixel coords from flattened:
		uint2 coords2D = unflatten2D(pixelID, GetInternalResolution());

		// Compute screen coordinates:
		float2 uv = float2((coords2D + xTracePixelOffset) * g_xFrame_InternalResolution_Inverse * 2.0f - 1.0f) * float2(1, -1);

		float seed = xTraceRandomSeed;

		float3 sampling_offset = float3(rand(seed, uv), rand(seed, uv), rand(seed, uv)) * 2 - 1;

		float3 finalResult = 0;

		TriangleData tri = TriangleData_Unpack(primitiveBuffer[ray.primitiveID], primitiveDataBuffer[ray.primitiveID]);

		float u = ray.bary.x;
		float v = ray.bary.y;
		float w = 1 - u - v;

		float3 P = ray.origin;
		float3 N = normalize(tri.n0 * w + tri.n1 * u + tri.n2 * v);
		float3 V = normalize(g_xFrame_MainCamera_CamPos - P);
		float4 uvsets = tri.u0 * w + tri.u1 * u + tri.u2 * v;
		float4 color = tri.c0 * w + tri.c1 * u + tri.c2 * v;


		uint materialIndex = tri.materialIndex;

		TracedRenderingMaterial material = materialBuffer[materialIndex];

		uvsets = frac(uvsets); // emulate wrap

		float4 baseColor;
		[branch]
		if (material.uvset_baseColorMap >= 0)
		{
			const float2 UV_baseColorMap = material.uvset_baseColorMap == 0 ? uvsets.xy : uvsets.zw;
			baseColor = materialTextureAtlas.SampleLevel(sampler_linear_clamp, UV_baseColorMap * material.baseColorAtlasMulAdd.xy + material.baseColorAtlasMulAdd.zw, 0);
			baseColor = DEGAMMA(baseColor);
		}
		else
		{
			baseColor = 1;
		}
		baseColor *= color;

		float4 surface_occlusion_roughness_metallic_reflectance;
		[branch]
		if (material.uvset_surfaceMap >= 0)
		{
			const float2 UV_surfaceMap = material.uvset_surfaceMap == 0 ? uvsets.xy : uvsets.zw;
			surface_occlusion_roughness_metallic_reflectance = materialTextureAtlas.SampleLevel(sampler_linear_clamp, UV_surfaceMap * material.surfaceMapAtlasMulAdd.xy + material.surfaceMapAtlasMulAdd.zw, 0);
			if (material.specularGlossinessWorkflow)
			{
				ConvertToSpecularGlossiness(surface_occlusion_roughness_metallic_reflectance);
			}
		}
		else
		{
			surface_occlusion_roughness_metallic_reflectance = 1;
		}

		[branch]
		if (material.uvset_normalMap >= 0)
		{
			const float2 UV_normalMap = material.uvset_normalMap == 0 ? uvsets.xy : uvsets.zw;
			float3 normalMap = materialTextureAtlas.SampleLevel(sampler_linear_clamp, UV_normalMap * material.normalMapAtlasMulAdd.xy + material.normalMapAtlasMulAdd.zw, 0).rgb;
			normalMap = normalMap.rgb * 2 - 1;
			normalMap.g *= material.normalMapFlip;
			const float3x3 TBN = float3x3(tri.tangent, tri.binormal, N);
			N = normalize(lerp(N, mul(normalMap, TBN), material.normalMapStrength));
		}

		float roughness = material.roughness * surface_occlusion_roughness_metallic_reflectance.g;
		float metalness = material.metalness * surface_occlusion_roughness_metallic_reflectance.b;
		float reflectance = material.reflectance * surface_occlusion_roughness_metallic_reflectance.a;

		Surface surface = CreateSurface(P, N, V, baseColor, 1, roughness, metalness, reflectance);

		[loop]
		for (uint iterator = 0; iterator < g_xFrame_LightArrayCount; iterator++)
		{
			ShaderEntityType light = EntityArray[g_xFrame_LightArrayOffset + iterator];
			Lighting lighting = CreateLighting(0, 0, 0, 0);
		
			float3 L = 0;
			float dist = 0;
		
			switch (light.GetType())
			{
			case ENTITY_TYPE_DIRECTIONALLIGHT:
			{
				dist = INFINITE_RAYHIT;

				float3 lightColor = light.GetColor().rgb*light.energy;

				L = light.directionWS.xyz;
				SurfaceToLight surfaceToLight = CreateSurfaceToLight(surface, L);

				lighting.direct.specular = lightColor * BRDF_GetSpecular(surface, surfaceToLight);
				lighting.direct.diffuse = lightColor * BRDF_GetDiffuse(surface, surfaceToLight);
			}
			break;
			case ENTITY_TYPE_POINTLIGHT:
			{
				L = light.positionWS - P;
				const float dist2 = dot(L, L);
				dist = sqrt(dist2);

				[branch]
				if (dist < light.range)
				{
					L /= dist;

					const float3 lightColor = light.GetColor().rgb*light.energy;

					SurfaceToLight surfaceToLight = CreateSurfaceToLight(surface, L);

					lighting.direct.specular = lightColor * BRDF_GetSpecular(surface, surfaceToLight);
					lighting.direct.diffuse = lightColor * BRDF_GetDiffuse(surface, surfaceToLight);

					const float range2 = light.range * light.range;
					const float att = saturate(1.0 - (dist2 / range2));
					const float attenuation = att * att;

					lighting.direct.diffuse *= attenuation;
					lighting.direct.specular *= attenuation;
				}
			}
			break;
			case ENTITY_TYPE_SPOTLIGHT:
			{
				L = light.positionWS - surface.P;
				const float dist2 = dot(L, L);
				dist = sqrt(dist2);

				[branch]
				if (dist < light.range)
				{
					L /= dist;

					const float3 lightColor = light.GetColor().rgb*light.energy;

					const float SpotFactor = dot(L, light.directionWS);
					const float spotCutOff = light.coneAngleCos;

					[branch]
					if (SpotFactor > spotCutOff)
					{
						SurfaceToLight surfaceToLight = CreateSurfaceToLight(surface, L);

						lighting.direct.specular = lightColor * BRDF_GetSpecular(surface, surfaceToLight);
						lighting.direct.diffuse = lightColor * BRDF_GetDiffuse(surface, surfaceToLight);

						const float range2 = light.range * light.range;
						const float att = saturate(1.0 - (dist2 / range2));
						float attenuation = att * att;
						attenuation *= saturate((1.0 - (1.0 - SpotFactor) * 1.0 / (1.0 - spotCutOff)));

						lighting.direct.diffuse *= attenuation;
						lighting.direct.specular *= attenuation;
					}
				}
			}
			break;
			case ENTITY_TYPE_SPHERELIGHT:
			{
			}
			break;
			case ENTITY_TYPE_DISCLIGHT:
			{
			}
			break;
			case ENTITY_TYPE_RECTANGLELIGHT:
			{
			}
			break;
			case ENTITY_TYPE_TUBELIGHT:
			{
			}
			break;
			}
		
			float NdotL = saturate(dot(L, N));
		
			if (NdotL > 0 && dist > 0)
			{
				lighting.direct.diffuse = max(0.0f, lighting.direct.diffuse);
				lighting.direct.specular = max(0.0f, lighting.direct.specular);

				Ray newRay;
				newRay.origin = P;
				newRay.direction = L + sampling_offset * 0.025f;
				newRay.direction_inverse = rcp(newRay.direction);
				newRay.energy = 0;
				bool hit = TraceSceneANY(newRay, dist);
				finalResult += (hit ? 0 : NdotL) * (lighting.direct.diffuse + lighting.direct.specular);
			}
		}
		
		finalResult *= ray.energy;

		resultTexture[coords2D] += float4(max(0, finalResult), 0);

	}

	// This shader doesn't export any rays!
}