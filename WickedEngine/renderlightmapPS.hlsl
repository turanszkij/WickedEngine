#define RAY_BACKFACE_CULLING
#include "globals.hlsli"
#include "raytracingHF.hlsli"

struct Input
{
	float4 pos : SV_POSITION;
	float2 uv : TEXCOORD;
	float3 pos3D : WORLDPOSITION;
	float3 normal : NORMAL;
};

float4 main(Input input) : SV_TARGET
{
	float3 P = input.pos3D;
	float3 N = normalize(input.normal);
	float2 uv = input.uv;
	float seed = xTraceRandomSeed;
	float3 direction = SampleHemisphere_cos(N, seed, uv);
	Ray ray = CreateRay(trace_bias_position(P, N), direction);

	const uint bounces = xTraceUserData.x;
	for (uint i = 0; (i < bounces) && any(ray.energy); ++i)
	{
		P = ray.origin;
		float3 bounceResult = 0;

		[loop]
		for (uint iterator = 0; iterator < g_xFrame_LightArrayCount; iterator++)
		{
			ShaderEntity light = EntityArray[g_xFrame_LightArrayOffset + iterator];

			Lighting lighting;
			lighting.create(0, 0, 0, 0);

			if (!(light.GetFlags() & ENTITY_FLAG_LIGHT_STATIC))
			{
				continue; // dynamic lights will not be baked into lightmap
			}

			float3 L = 0;
			float dist = 0;
			float NdotL = 0;

			switch (light.GetType())
			{
			case ENTITY_TYPE_DIRECTIONALLIGHT:
			{
				dist = FLT_MAX;

				L = light.GetDirection().xyz; 
				NdotL = saturate(dot(L, N));

				[branch]
				if (NdotL > 0)
				{
					float3 atmosphereTransmittance = 1.0;
					if (g_xFrame_Options & OPTION_BIT_REALISTIC_SKY)
					{
						AtmosphereParameters Atmosphere = GetAtmosphereParameters();
						atmosphereTransmittance = GetAtmosphericLightTransmittance(Atmosphere, P, L, texture_transmittancelut);
					}
					
					float3 lightColor = light.GetColor().rgb * light.GetEnergy() * atmosphereTransmittance;

					lighting.direct.diffuse = lightColor;
				}
			}
			break;
			case ENTITY_TYPE_POINTLIGHT:
			{
				L = light.position - P;
				const float dist2 = dot(L, L);
				const float range2 = light.GetRange() * light.GetRange();

				[branch]
				if (dist2 < range2)
				{
					dist = sqrt(dist2);
					L /= dist; 
					NdotL = saturate(dot(L, N));

					[branch]
					if (NdotL > 0)
					{
						const float3 lightColor = light.GetColor().rgb * light.GetEnergy();

						lighting.direct.diffuse = lightColor;

						const float range2 = light.GetRange() * light.GetRange();
						const float att = saturate(1.0 - (dist2 / range2));
						const float attenuation = att * att;

						lighting.direct.diffuse *= attenuation;
					}
				}
			}
			break;
			case ENTITY_TYPE_SPOTLIGHT:
			{
				L = light.position - P;
				const float dist2 = dot(L, L);
				const float range2 = light.GetRange() * light.GetRange();

				[branch]
				if (dist2 < range2)
				{
					dist = sqrt(dist2);
					L /= dist;
					NdotL = saturate(dot(L, N));

					[branch]
					if (NdotL > 0)
					{
						const float SpotFactor = dot(L, light.GetDirection());
						const float spotCutOff = light.GetConeAngleCos();

						[branch]
						if (SpotFactor > spotCutOff)
						{
							const float3 lightColor = light.GetColor().rgb * light.GetEnergy();

							lighting.direct.diffuse = lightColor;

							const float range2 = light.GetRange() * light.GetRange();
							const float att = saturate(1.0 - (dist2 / range2));
							float attenuation = att * att;
							attenuation *= saturate((1.0 - (1.0 - SpotFactor) * 1.0 / (1.0 - spotCutOff)));

							lighting.direct.diffuse *= attenuation;
						}
					}
				}
			}
			break;
			}

			if (NdotL > 0 && dist > 0)
			{
				lighting.direct.diffuse = max(0.0f, lighting.direct.diffuse);

				float3 sampling_offset = float3(rand(seed, uv), rand(seed, uv), rand(seed, uv)) * 2 - 1;

				Ray newRay;
				newRay.origin = trace_bias_position(P, N);
				newRay.direction = L + sampling_offset * 0.025f;
				newRay.direction_rcp = rcp(newRay.direction);
				newRay.energy = 0;
				bool hit = TraceRay_Any(newRay, dist);
				bounceResult += (hit ? 0 : NdotL) * lighting.direct.diffuse / PI;
			}
		}
		ray.color += max(0, ray.energy * bounceResult);

		// Sample primary ray (scene materials, sky, etc):
		RayHit hit = TraceRay_Closest(ray);

		if (hit.distance >= FLT_MAX - 1)
		{
			float3 envColor;
			[branch]
			if (IsStaticSky())
			{
				// We have envmap information in a texture:
				envColor = DEGAMMA_SKY(texture_globalenvmap.SampleLevel(sampler_linear_clamp, ray.direction, 0).rgb);
			}
			else
			{
				envColor = GetDynamicSkyColor(ray.direction, true, true, false, true);
			}
			ray.color += max(0, ray.energy * envColor);

			// Erase the ray's energy
			ray.energy = 0.0f;
			break;
		}

		ray.origin = hit.position;
		ray.primitiveID = hit.primitiveID;
		ray.bary = hit.bary;

		TriangleData tri = TriangleData_Unpack(primitiveBuffer[ray.primitiveID], primitiveDataBuffer[ray.primitiveID]);

		float u = ray.bary.x;
		float v = ray.bary.y;
		float w = 1 - u - v;

		N = normalize(tri.n0 * w + tri.n1 * u + tri.n2 * v);
		float4 uvsets = tri.u0 * w + tri.u1 * u + tri.u2 * v;
		float4 color = tri.c0 * w + tri.c1 * u + tri.c2 * v;
		uint materialIndex = tri.materialIndex;

		ShaderMaterial material = materialBuffer[materialIndex];

		uvsets = frac(uvsets); // emulate wrap

		float4 baseColor;
		[branch]
		if (material.uvset_baseColorMap >= 0 && (g_xFrame_Options & OPTION_BIT_DISABLE_ALBEDO_MAPS) == 0)
		{
			const float2 UV_baseColorMap = material.uvset_baseColorMap == 0 ? uvsets.xy : uvsets.zw;
			baseColor = materialTextureAtlas.SampleLevel(sampler_linear_clamp, UV_baseColorMap * material.baseColorAtlasMulAdd.xy + material.baseColorAtlasMulAdd.zw, 0);
			baseColor.rgb = DEGAMMA(baseColor.rgb);
		}
		else
		{
			baseColor = 1;
		}
		baseColor *= color;

		float4 surfaceMap = 1;
		[branch]
		if (material.uvset_surfaceMap >= 0)
		{
			const float2 UV_surfaceMap = material.uvset_surfaceMap == 0 ? uvsets.xy : uvsets.zw;
			surfaceMap = materialTextureAtlas.SampleLevel(sampler_linear_clamp, UV_surfaceMap * material.surfaceMapAtlasMulAdd.xy + material.surfaceMapAtlasMulAdd.zw, 0);
		}

		Surface surface;
		surface.create(material, baseColor, surfaceMap);

		surface.emissiveColor = material.emissiveColor;
		[branch]
		if (material.emissiveColor.a > 0 && material.uvset_emissiveMap >= 0)
		{
			const float2 UV_emissiveMap = material.uvset_emissiveMap == 0 ? uvsets.xy : uvsets.zw;
			float4 emissiveMap = materialTextureAtlas.SampleLevel(sampler_linear_clamp, UV_emissiveMap * material.emissiveMapAtlasMulAdd.xy + material.emissiveMapAtlasMulAdd.zw, 0);
			emissiveMap.rgb = DEGAMMA(emissiveMap.rgb);
			surface.emissiveColor *= emissiveMap;
		}

		ray.color += max(0, ray.energy * surface.emissiveColor.rgb * surface.emissiveColor.a);

		[branch]
		if (material.uvset_normalMap >= 0)
		{
			const float2 UV_normalMap = material.uvset_normalMap == 0 ? uvsets.xy : uvsets.zw;
			float3 normalMap = materialTextureAtlas.SampleLevel(sampler_linear_clamp, UV_normalMap * material.normalMapAtlasMulAdd.xy + material.normalMapAtlasMulAdd.zw, 0).rgb;
			normalMap = normalMap.rgb * 2 - 1;
			const float3x3 TBN = float3x3(tri.tangent, tri.binormal, N);
			N = normalize(lerp(N, mul(normalMap, TBN), material.normalMapStrength));
		}

		// Calculate chances of reflection types:
		const float refractChance = 1 - baseColor.a;

		// Roughness to cone aperture:
		float alphaRoughness = surface.roughness * surface.roughness;

		// Roulette-select the ray's path
		float roulette = rand(seed, uv);
		if (roulette < refractChance)
		{
			// Refraction
			const float3 R = refract(ray.direction, N, 1 - material.refraction);
			ray.direction = lerp(R, SampleHemisphere_cos(R, seed, uv), alphaRoughness);
			ray.energy *= lerp(baseColor.rgb, 1, refractChance);

			// The ray penetrates the surface, so push DOWN along normal to avoid self-intersection:
			ray.origin = trace_bias_position(ray.origin, -N);
		}
		else
		{
			// Calculate chances of reflection types:
			const float3 F = F_Schlick(surface.f0, saturate(dot(-ray.direction, N)));
			const float specChance = dot(F, 0.333f);

			roulette = rand(seed, uv);
			if (roulette < specChance)
			{
				// Specular reflection
				const float3 R = reflect(ray.direction, N);
				ray.direction = lerp(R, SampleHemisphere_cos(R, seed, uv), alphaRoughness);
				ray.energy *= F / specChance;
			}
			else
			{
				// Diffuse reflection
				ray.direction = SampleHemisphere_cos(N, seed, uv);
				ray.energy *= surface.albedo / (1 - specChance);
			}

			// Ray reflects from surface, so push UP along normal to avoid self-intersection:
			ray.origin = trace_bias_position(ray.origin, N);
		}

		ray.Update();
	}

	return float4(ray.color, xTraceAccumulationFactor);
}
