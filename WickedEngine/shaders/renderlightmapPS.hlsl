#define RAY_BACKFACE_CULLING
#include "globals.hlsli"
#include "raytracingHF.hlsli"

#ifdef RTAPI
RAYTRACINGACCELERATIONSTRUCTURE(scene_acceleration_structure, TEXSLOT_ACCELERATION_STRUCTURE);
Texture2D<float4> bindless_textures[] : register(t0, space1);
ByteAddressBuffer bindless_buffers[] : register(t0, space2);
StructuredBuffer<ShaderMeshSubset> bindless_subsets[] : register(t0, space3);
Buffer<uint> bindless_ib[] : register(t0, space4);
#endif // RTAPI

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
	float3 result = 0;

	uint bounces = xTraceUserData.x;
	const uint bouncelimit = 16;
	for (uint bounce = 0; ((bounce < min(bounces, bouncelimit)) && any(ray.energy)); ++bounce)
	{
		P = ray.origin;

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
				newRay.energy = 0;
				newRay.Update();
#ifdef RTAPI
				RayDesc apiray;
				apiray.TMin = 0.001;
				apiray.TMax = dist;
				apiray.Origin = newRay.origin;
				apiray.Direction = newRay.direction;
				RayQuery<
					RAY_FLAG_FORCE_OPAQUE |
					RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES |
					RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH
				> q;
				q.TraceRayInline(scene_acceleration_structure, 0, 0xFF, apiray);
				q.Proceed();
				bool hit = q.CommittedStatus() == COMMITTED_TRIANGLE_HIT;
#else
				bool hit = TraceRay_Any(newRay, dist);
#endif // RTAPI
				result += max(0, ray.energy * (hit ? 0 : NdotL) * lighting.direct.diffuse / PI);
			}
		}

		// Sample primary ray (scene materials, sky, etc):

#ifdef RTAPI
		RayDesc apiray;
		apiray.TMin = 0.001;
		apiray.TMax = FLT_MAX;
		apiray.Origin = ray.origin;
		apiray.Direction = ray.direction;
		RayQuery<
			RAY_FLAG_FORCE_OPAQUE |
			RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES
		> q;
		q.TraceRayInline(scene_acceleration_structure, 0, 0xFF, apiray);
		q.Proceed();
		if(q.CommittedStatus() != COMMITTED_TRIANGLE_HIT)
#else
		RayHit hit = TraceRay_Closest(ray);

		if (hit.distance >= FLT_MAX - 1)
#endif // RTAPI

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
			result += max(0, ray.energy * envColor);

			// Erase the ray's energy
			ray.energy = 0;
			break;
		}

#ifdef RTAPI

		// ray origin updated for next bounce:
		ray.origin = q.WorldRayOrigin() + q.WorldRayDirection() * q.CommittedRayT();

		// RTAPI path: bindless
		ShaderMesh mesh = bindless_buffers[q.CommittedInstanceID()].Load<ShaderMesh>(0);
		ShaderMeshSubset subset = bindless_subsets[mesh.subsetbuffer][q.CommittedGeometryIndex()];
		ShaderMaterial material = bindless_buffers[subset.material].Load<ShaderMaterial>(0);
		uint startIndex = q.CommittedPrimitiveIndex() * 3 + subset.indexOffset;
		uint i0 = bindless_ib[mesh.ib][startIndex + 0];
		uint i1 = bindless_ib[mesh.ib][startIndex + 1];
		uint i2 = bindless_ib[mesh.ib][startIndex + 2];
		float4 uv0 = 0, uv1 = 0, uv2 = 0;
		[branch]
		if (mesh.vb_uv0 >= 0)
		{
			uv0.xy = unpack_half2(bindless_buffers[mesh.vb_uv0].Load(i0 * 4));
			uv1.xy = unpack_half2(bindless_buffers[mesh.vb_uv0].Load(i1 * 4));
			uv2.xy = unpack_half2(bindless_buffers[mesh.vb_uv0].Load(i2 * 4));
		}
		[branch]
		if (mesh.vb_uv1 >= 0)
		{
			uv0.zw = unpack_half2(bindless_buffers[mesh.vb_uv1].Load(i0 * 4));
			uv1.zw = unpack_half2(bindless_buffers[mesh.vb_uv1].Load(i1 * 4));
			uv2.zw = unpack_half2(bindless_buffers[mesh.vb_uv1].Load(i2 * 4));
		}
		float3 n0 = 0, n1 = 0, n2 = 0;
		[branch]
		if (mesh.vb_pos_nor_wind >= 0)
		{
			const uint stride_POS = 16;
			n0 = unpack_unitvector(bindless_buffers[mesh.vb_pos_nor_wind].Load4(i0 * stride_POS).w);
			n1 = unpack_unitvector(bindless_buffers[mesh.vb_pos_nor_wind].Load4(i1 * stride_POS).w);
			n2 = unpack_unitvector(bindless_buffers[mesh.vb_pos_nor_wind].Load4(i2 * stride_POS).w);
		}
		else
		{
			return float4(1, 0, 1, 1); // error, this should always be good
		}

		float2 barycentrics = q.CommittedTriangleBarycentrics();
		float u = barycentrics.x;
		float v = barycentrics.y;
		float w = 1 - u - v;
		float4 uvsets = uv0 * w + uv1 * u + uv2 * v;
		float3 N = n0 * w + n1 * u + n2 * v;

		N = mul((float3x3)q.CommittedObjectToWorld3x4(), N);
		N = normalize(N);

		float4 baseColor = material.baseColor;
		[branch]
		if (material.texture_basecolormap_index >= 0 && (g_xFrame_Options & OPTION_BIT_DISABLE_ALBEDO_MAPS) == 0)
		{
			const float2 UV_baseColorMap = material.uvset_baseColorMap == 0 ? uvsets.xy : uvsets.zw;
			baseColor = bindless_textures[material.texture_basecolormap_index].SampleLevel(sampler_linear_wrap, UV_baseColorMap, 2);
			baseColor.rgb *= DEGAMMA(baseColor.rgb);
		}

		[branch]
		if (mesh.vb_col >= 0 && material.IsUsingVertexColors())
		{
			float4 c0, c1, c2;
			const uint stride_COL = 4;
			c0 = unpack_rgba(bindless_buffers[mesh.vb_col].Load(i0 * stride_COL));
			c1 = unpack_rgba(bindless_buffers[mesh.vb_col].Load(i1 * stride_COL));
			c2 = unpack_rgba(bindless_buffers[mesh.vb_col].Load(i2 * stride_COL));
			float4 vertexColor = c0 * w + c1 * u + c2 * v;
			baseColor *= vertexColor;
		}

		[branch]
		if (mesh.vb_tan >= 0 && material.texture_normalmap_index >= 0 && material.normalMapStrength > 0)
		{
			float4 t0, t1, t2;
			const uint stride_TAN = 4;
			t0 = unpack_utangent(bindless_buffers[mesh.vb_tan].Load(i0 * stride_TAN));
			t1 = unpack_utangent(bindless_buffers[mesh.vb_tan].Load(i1 * stride_TAN));
			t2 = unpack_utangent(bindless_buffers[mesh.vb_tan].Load(i2 * stride_TAN));
			float4 T = t0 * w + t1 * u + t2 * v;
			T = T * 2 - 1;
			T.xyz = mul((float3x3)q.CommittedObjectToWorld3x4(), T.xyz);
			T.xyz = normalize(T.xyz);
			float3 B = normalize(cross(T.xyz, N) * T.w);
			float3x3 TBN = float3x3(T.xyz, B, N);

			const float2 UV_normalMap = material.uvset_normalMap == 0 ? uvsets.xy : uvsets.zw;
			float3 normalMap = bindless_textures[material.texture_normalmap_index].SampleLevel(sampler_linear_wrap, UV_normalMap, 2).rgb;
			normalMap = normalMap * 2 - 1;
			N = normalize(lerp(N, mul(normalMap, TBN), material.normalMapStrength));
		}

		float4 surfaceMap = 1;
		[branch]
		if (material.texture_surfacemap_index >= 0)
		{
			const float2 UV_surfaceMap = material.uvset_surfaceMap == 0 ? uvsets.xy : uvsets.zw;
			surfaceMap = bindless_textures[material.texture_surfacemap_index].SampleLevel(sampler_linear_wrap, UV_surfaceMap, 2);
		}

		Surface surface;
		surface.create(material, baseColor, surfaceMap);

		surface.emissiveColor = material.emissiveColor;
		[branch]
		if (material.texture_emissivemap_index >= 0)
		{
			const float2 UV_emissiveMap = material.uvset_emissiveMap == 0 ? uvsets.xy : uvsets.zw;
			float4 emissiveMap = bindless_textures[material.texture_emissivemap_index].SampleLevel(sampler_linear_wrap, UV_emissiveMap, 2);
			emissiveMap.rgb = DEGAMMA(emissiveMap.rgb);
			surface.emissiveColor *= emissiveMap;
		}

#else

		// Non-RTAPI path: sampling from texture atlas
		ray.origin = hit.position;

		TriangleData tri = TriangleData_Unpack(primitiveBuffer[hit.primitiveID], primitiveDataBuffer[hit.primitiveID]);

		float u = hit.bary.x;
		float v = hit.bary.y;
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

		[branch]
		if (material.uvset_normalMap >= 0)
		{
			const float2 UV_normalMap = material.uvset_normalMap == 0 ? uvsets.xy : uvsets.zw;
			float3 normalMap = materialTextureAtlas.SampleLevel(sampler_linear_clamp, UV_normalMap * material.normalMapAtlasMulAdd.xy + material.normalMapAtlasMulAdd.zw, 0).rgb;
			normalMap = normalMap.rgb * 2 - 1;
			const float3x3 TBN = float3x3(tri.tangent, tri.binormal, N);
			N = normalize(lerp(N, mul(normalMap, TBN), material.normalMapStrength));
		}
#endif // RTAPI

		surface.update();

		result += max(0, ray.energy * surface.emissiveColor.rgb * surface.emissiveColor.a);

		// Calculate chances of reflection types:
		const float refractChance = material.transmission;

		// Roulette-select the ray's path
		float roulette = rand(seed, uv);
		if (roulette < refractChance)
		{
			// Refraction
			const float3 R = refract(ray.direction, N, 1 - material.refraction);
			ray.direction = lerp(R, SampleHemisphere_cos(R, seed, uv), surface.roughnessBRDF);
			ray.energy *= surface.albedo;

			// The ray penetrates the surface, so push DOWN along normal to avoid self-intersection:
			ray.origin = trace_bias_position(ray.origin, -N);

			// Add a new bounce iteration, otherwise the transparent effect can disappear:
			bounces++;
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
				ray.direction = lerp(R, SampleHemisphere_cos(R, seed, uv), surface.roughnessBRDF);
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

	return float4(result, xTraceAccumulationFactor);
}
