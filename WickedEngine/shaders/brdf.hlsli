#ifndef WI_BRDF_HF
#define WI_BRDF_HF
#include "globals.hlsli"

// BRDF functions source: https://github.com/google/filament/blob/main/shaders/src/brdf.fs

#define MEDIUMP_FLT_MAX    65504.0
#define saturateMediump(x) min(x, MEDIUMP_FLT_MAX)
#define highp
#define pow5(x) pow(x, 5)
#define max3(v) max(max(v.x, v.y), v.z)

float D_GGX(float roughness, float NoH, const float3 h)
{
	// Walter et al. 2007, "Microfacet Models for Refraction through Rough Surfaces"
	float oneMinusNoHSquared = 1.0 - NoH * NoH;

	float a = NoH * roughness;
	float k = roughness / (oneMinusNoHSquared + a * a);
	float d = k * k * (1.0 / PI);
	return saturateMediump(d);
}

float D_GGX_Anisotropic(float at, float ab, float ToH, float BoH, float NoH)
{
	// Burley 2012, "Physically-Based Shading at Disney"

	// The values at and ab are perceptualRoughness^2, a2 is therefore perceptualRoughness^4
	// The dot product below computes perceptualRoughness^8. We cannot fit in fp16 without clamping
	// the roughness to too high values so we perform the dot product and the division in fp32
	float a2 = at * ab;
	highp float3 d = float3(ab * ToH, at * BoH, a2 * NoH);
	highp float d2 = dot(d, d);
	float b2 = a2 / d2;
	return a2 * b2 * b2 * (1.0 / PI);
}

float D_Charlie(float roughness, float NoH)
{
	// Estevez and Kulla 2017, "Production Friendly Microfacet Sheen BRDF"
	float invAlpha = 1.0 / roughness;
	float cos2h = NoH * NoH;
	float sin2h = max(1.0 - cos2h, 0.0078125); // 2^(-14/2), so sin2h^2 > 0 in fp16
	return (2.0 + invAlpha) * pow(sin2h, invAlpha * 0.5) / (2.0 * PI);
}

float V_SmithGGXCorrelated(float roughness, float NoV, float NoL)
{
	// Heitz 2014, "Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs"
	float a2 = roughness * roughness;
	// TODO: lambdaV can be pre-computed for all the lights, it should be moved out of this function
	float lambdaV = NoL * sqrt((NoV - a2 * NoV) * NoV + a2);
	float lambdaL = NoV * sqrt((NoL - a2 * NoL) * NoL + a2);
	float v = 0.5 / (lambdaV + lambdaL);
	// a2=0 => v = 1 / 4*NoL*NoV   => min=1/4, max=+inf
	// a2=1 => v = 1 / 2*(NoL+NoV) => min=1/4, max=+inf
	// clamp to the maximum value representable in mediump
	return saturateMediump(v);
}

float V_SmithGGXCorrelated_Anisotropic(float at, float ab, float ToV, float BoV,
	float ToL, float BoL, float NoV, float NoL)
{
	// Heitz 2014, "Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs"
	// TODO: lambdaV can be pre-computed for all the lights, it should be moved out of this function
	float lambdaV = NoL * length(float3(at * ToV, ab * BoV, NoV));
	float lambdaL = NoV * length(float3(at * ToL, ab * BoL, NoL));
	float v = 0.5 / (lambdaV + lambdaL);
	return saturateMediump(v);
}

float V_Kelemen(float LoH)
{
	// Kelemen 2001, "A Microfacet Based Coupled Specular-Matte BRDF Model with Importance Sampling"
	return saturateMediump(0.25 / (LoH * LoH));
}

float V_Neubelt(float NoV, float NoL)
{
	// Neubelt and Pettineo 2013, "Crafting a Next-gen Material Pipeline for The Order: 1886"
	return saturateMediump(1.0 / (4.0 * (NoL + NoV - NoL * NoV)));
}

float3 F_Schlick(const float3 f0, float f90, float VoH)
{
	// Schlick 1994, "An Inexpensive BRDF Model for Physically-Based Rendering"
	return f0 + (f90 - f0) * pow5(1.0 - VoH);
}

float iorToF0(float transmittedIor, float incidentIor)
{
	return sqr((transmittedIor - incidentIor) / (transmittedIor + incidentIor));
}

float f0ToIor(float f0)
{
	float r = sqrt(f0);
	return (1.0 + r) / (1.0 - r);
}


// Surface descriptors:

struct SheenSurface
{
	float3 color;
	float roughness;

	// computed values:
	float roughnessBRDF;
	float DFG;
	float albedoScaling;
};

struct ClearcoatSurface
{
	float factor;
	float roughness;
	float3 N;

	// computed values:
	float roughnessBRDF;
	float3 R;
	float3 F;
};

enum
{
	SURFACE_FLAG_BACKFACE = 1u << 0u,
	SURFACE_FLAG_RECEIVE_SHADOW = 1u << 1u,
	SURFACE_FLAG_GI_APPLIED = 1u << 2u,
};

struct Surface
{
	// Fill these yourself:
	float3 P;				// world space position
	float3 N;				// world space normal
	float3 V;				// world space view vector

	float3 albedo;			// diffuse light absorbtion value (rgb)
	float3 f0;				// fresnel value (rgb) (reflectance at incidence angle, also known as specular color)
	float roughness;		// roughness: [0:smooth -> 1:rough] (perceptual)
	float occlusion;		// occlusion [0 -> 1]
	float opacity;			// opacity for blending operation [0 -> 1]
	float3 emissiveColor;	// light emission [0 -> 1]
	float4 refraction;		// refraction color (rgb), refraction amount (a)
	float transmission;		// transmission factor
	float2 pixel;			// pixel coordinate (used for randomization effects)
	float2 screenUV;		// pixel coordinate in UV space [0 -> 1] (used for randomization effects)
	float3 T;				// tangent
	float3 B;				// bitangent
	float anisotropy;		// anisotropy factor [0 -> 1]
	float4 sss;				// subsurface scattering color * amount
	float4 sss_inv;			// 1 / (1 + sss)
	uint layerMask;			// the engine-side layer mask
	float3 facenormal;		// surface normal without normal map
	uint flags;
	uint uid_validate;
	RayCone raycone;
	float hit_depth;

	// These will be computed when calling Update():
	float roughnessBRDF;	// roughness input for BRDF functions
	float NdotV;			// cos(angle between normal and view vector)
	float f90;				// reflectance at grazing angle
	float3 R;				// reflection vector
	float3 F;				// fresnel term computed from NdotV
	float TdotV;
	float BdotV;
	float at;
	float ab;

	SheenSurface sheen;
	ClearcoatSurface clearcoat;

	inline void init()
	{
		P = 0;
		V = 0;
		N = 0;
		albedo = 1;
		f0 = 0;
		roughness = 1;
		occlusion = 1;
		opacity = 1;
		emissiveColor = 0;
		refraction = 0;
		transmission = 0;
		pixel = 0;
		screenUV = 0;
		T = 0;
		B = 0;
		anisotropy = 0;
		sss = 0;
		sss_inv = 1;
		layerMask = ~0;
		facenormal = 0;
		flags = 0;

		sheen.color = 0;
		sheen.roughness = 0;

		clearcoat.factor = 0;
		clearcoat.roughness = 0;
		clearcoat.N = 0;

		uid_validate = 0;
		raycone = (RayCone)0;
		hit_depth = 0;
	}

	inline void create(
		in ShaderMaterial material,
		in float4 baseColor,
		in float4 surfaceMap,
		in float4 specularMap = 1
	)
	{
		if (material.options & SHADERMATERIAL_OPTION_BIT_TRANSPARENT || material.alphaTest > 0)
		{
			opacity = baseColor.a;
		}
		else
		{
			opacity = 1;
		}
		roughness = material.roughness;
		f0 = material.GetSpecular() * specularMap.rgb * specularMap.a;

		if (GetFrame().options & OPTION_BIT_FORCE_DIFFUSE_LIGHTING)
		{
			f0 = material.metalness = material.reflectance = 0;
		}

		[branch]
		if (material.IsUsingSpecularGlossinessWorkflow())
		{
			// Specular-glossiness workflow:
			roughness *= saturate(1 - surfaceMap.a);
			f0 *= DEGAMMA(surfaceMap.rgb);
			albedo = baseColor.rgb;
		}
		else
		{
			// Metallic-roughness workflow:
			if (material.IsOcclusionEnabled_Primary())
			{
				occlusion = surfaceMap.r;
			}
			roughness *= surfaceMap.g;
			const float metalness = material.metalness * surfaceMap.b;
			const float reflectance = material.reflectance * surfaceMap.a;
			albedo = baseColor.rgb * (1 - max(reflectance, metalness));
			f0 *= lerp(reflectance.xxx, baseColor.rgb, metalness);
		}

		if (material.IsReceiveShadow())
		{
			flags |= SURFACE_FLAG_RECEIVE_SHADOW;
		}
	}

	inline void update()
	{
		roughness = clamp(roughness, 0.045, 1);
		roughnessBRDF = roughness * roughness;

		sheen.roughness = clamp(sheen.roughness, 0.045, 1);
		sheen.roughnessBRDF = sheen.roughness * sheen.roughness;

		clearcoat.roughness = clamp(clearcoat.roughness, 0.045, 1);
		clearcoat.roughnessBRDF = clearcoat.roughness * clearcoat.roughness;

		NdotV = saturate(abs(dot(N, V)) + 1e-5);

		f90 = saturate(50.0 * dot(f0, 0.33));
		F = F_Schlick(f0, f90, NdotV);
		clearcoat.F = F_Schlick(f0, f90, saturate(abs(dot(clearcoat.N, V)) + 1e-5));
		clearcoat.F *= clearcoat.factor;

		R = -reflect(V, N);
		clearcoat.R = -reflect(V, clearcoat.N);

		// Sheen energy compensation: https://dassaultsystemes-technology.github.io/EnterprisePBRShadingModel/spec-2021x.md.html#figure_energy-compensation-sheen-e
		sheen.DFG = texture_sheenlut.SampleLevel(sampler_linear_clamp, float2(NdotV, sheen.roughness), 0).r;
		sheen.albedoScaling = 1.0 - max3(sheen.color) * sheen.DFG;

		TdotV = dot(T, V);
		BdotV = dot(B, V);
		at = max(0, roughnessBRDF * (1 + anisotropy));
		ab = max(0, roughnessBRDF * (1 - anisotropy));

#ifdef BRDF_CARTOON
		F = smoothstep(0.05, 0.1, F);
#endif // BRDF_CARTOON
	}

	inline bool IsReceiveShadow() { return flags & SURFACE_FLAG_RECEIVE_SHADOW; }


	ShaderMeshInstance inst;
	ShaderGeometry geometry;
	ShaderMaterial material;
	float2 bary;
	uint i0;
	uint i1;
	uint i2;
	uint4 data0;
	uint4 data1;
	uint4 data2;
	float3 pre;

	bool preload_internal(PrimitiveID prim)
	{
		inst = load_instance(prim.instanceIndex);
		if (uid_validate != 0 && inst.uid != uid_validate)
			return false;

		geometry = load_geometry(inst.geometryOffset + prim.subsetIndex);
		if (geometry.vb_pos_nor_wind < 0)
			return false;

		const uint startIndex = prim.primitiveIndex * 3 + geometry.indexOffset;
		Buffer<uint> indexBuffer = bindless_ib[NonUniformResourceIndex(geometry.ib)];
		i0 = indexBuffer[startIndex + 0];
		i1 = indexBuffer[startIndex + 1];
		i2 = indexBuffer[startIndex + 2];

		ByteAddressBuffer buf = bindless_buffers[NonUniformResourceIndex(geometry.vb_pos_nor_wind)];
		data0 = buf.Load4(i0 * sizeof(uint4));
		data1 = buf.Load4(i1 * sizeof(uint4));
		data2 = buf.Load4(i2 * sizeof(uint4));

		return true;
	}
	void load_internal()
	{
		material = load_material(geometry.materialIndex);
		SamplerState sam = bindless_samplers[GetFrame().sampler_objectshader_index];

		const bool is_hairparticle = geometry.flags & SHADERMESH_FLAG_HAIRPARTICLE;
		const bool is_emittedparticle = geometry.flags & SHADERMESH_FLAG_EMITTEDPARTICLE;
		const bool simple_lighting = is_hairparticle || is_emittedparticle;

		const float u = bary.x;
		const float v = bary.y;
		const float w = 1 - u - v;

		float3 n0 = unpack_unitvector(data0.w);
		float3 n1 = unpack_unitvector(data1.w);
		float3 n2 = unpack_unitvector(data2.w);
		N = mad(n0, w, mad(n1, u, n2 * v)); // n0 * w + n1 * u + n2 * v
		N = mul((float3x3)inst.transformInverseTranspose.GetMatrix(), N);
		N = normalize(N);
		if ((flags & SURFACE_FLAG_BACKFACE) && !is_hairparticle && !is_emittedparticle)
		{
			N = -N;
		}
		facenormal = N;

#ifdef SURFACE_LOAD_MIPCONE
		float3 p0 = asfloat(data0.xyz);
		float3 p1 = asfloat(data1.xyz);
		float3 p2 = asfloat(data2.xyz);
		float3 P0 = mul(inst.transform.GetMatrix(), float4(p0, 1)).xyz;
		float3 P1 = mul(inst.transform.GetMatrix(), float4(p1, 1)).xyz;
		float3 P2 = mul(inst.transform.GetMatrix(), float4(p2, 1)).xyz;
		const float triangle_constant = rcp(twice_triangle_area(P0, P1, P2));
		float lod_constant0 = 0;
		float lod_constant1 = 0;
		const float3 ray_direction = V;
		const float cone_width = raycone.width_at_t(hit_depth);
		//const float3 surf_normal = facenormal;
		const float3 surf_normal = normalize(cross(P2 - P1, P1 - P0)); // compute the facenormal, because particles could have fake facenormal which doesn't work well with mipcones!
#endif // SURFACE_LOAD_MIPCONE

		float4 uvsets = 0;
		[branch]
		if (geometry.vb_uvs >= 0)
		{
			ByteAddressBuffer buf = bindless_buffers[NonUniformResourceIndex(geometry.vb_uvs)];
			const float4 uv0 = unpack_half4(buf.Load2(i0 * sizeof(uint2)));
			const float4 uv1 = unpack_half4(buf.Load2(i1 * sizeof(uint2)));
			const float4 uv2 = unpack_half4(buf.Load2(i2 * sizeof(uint2)));
			uvsets = mad(uv0, w, mad(uv1, u, uv2 * v)); // uv0 * w + uv1 * u + uv2 * v
			uvsets.xy = mad(uvsets.xy, material.texMulAdd.xy, material.texMulAdd.zw);

#ifdef SURFACE_LOAD_MIPCONE
			lod_constant0 = 0.5 * log2(twice_uv_area(uv0.xy, uv1.xy, uv2.xy) * triangle_constant);
			lod_constant1 = 0.5 * log2(twice_uv_area(uv0.zw, uv1.zw, uv2.zw) * triangle_constant);
#endif // SURFACE_LOAD_MIPCONE
		}

		float4 baseColor = is_emittedparticle ? 1 : material.baseColor;
		baseColor *= unpack_rgba(inst.color);
		[branch]
		if (material.texture_basecolormap_index >= 0)
		{
			const float2 UV_baseColorMap = material.uvset_baseColorMap == 0 ? uvsets.xy : uvsets.zw;
			Texture2D tex = bindless_textures[NonUniformResourceIndex(material.texture_basecolormap_index)];
			float lod = 0;
#ifdef SURFACE_LOAD_MIPCONE
			lod = compute_texture_lod(tex, material.uvset_baseColorMap == 0 ? lod_constant0 : lod_constant1, ray_direction, surf_normal, cone_width);
#endif // SURFACE_LOAD_MIPCONE
			float4 baseColorMap = tex.SampleLevel(sam, UV_baseColorMap, lod);
			if ((GetFrame().options & OPTION_BIT_DISABLE_ALBEDO_MAPS) == 0)
			{
				baseColorMap.rgb *= DEGAMMA(baseColorMap.rgb);
				baseColor *= baseColorMap;
			}
			else
			{
				baseColor.a *= baseColorMap.a;
			}
		}

		[branch]
		if (geometry.vb_col >= 0 && material.IsUsingVertexColors())
		{
			ByteAddressBuffer buf = bindless_buffers[NonUniformResourceIndex(geometry.vb_col)];
			const float4 c0 = unpack_rgba(buf.Load(i0 * sizeof(uint)));
			const float4 c1 = unpack_rgba(buf.Load(i1 * sizeof(uint)));
			const float4 c2 = unpack_rgba(buf.Load(i2 * sizeof(uint)));
			float4 vertexColor = mad(c0, w, mad(c1, u, c2 * v)); // c0 * w + c1 * u + c2 * v
			baseColor *= vertexColor;
		}

		float4 surfaceMap = 1;
		[branch]
		if (material.texture_surfacemap_index >= 0 && !simple_lighting)
		{
			const float2 UV_surfaceMap = material.uvset_surfaceMap == 0 ? uvsets.xy : uvsets.zw;
			Texture2D tex = bindless_textures[NonUniformResourceIndex(material.texture_surfacemap_index)];
			float lod = 0;
#ifdef SURFACE_LOAD_MIPCONE
			lod = compute_texture_lod(tex, material.uvset_surfaceMap == 0 ? lod_constant0 : lod_constant1, ray_direction, surf_normal, cone_width);
#endif // SURFACE_LOAD_MIPCONE
			surfaceMap = tex.SampleLevel(sam, UV_surfaceMap, lod);
		}
		if (simple_lighting)
		{
			surfaceMap = 0;
		}

		float4 specularMap = 1;
		[branch]
		if (material.texture_specularmap_index >= 0 && !simple_lighting)
		{
			const float2 UV_specularMap = material.uvset_specularMap == 0 ? uvsets.xy : uvsets.zw;
			Texture2D tex = bindless_textures[NonUniformResourceIndex(material.texture_specularmap_index)];
			float lod = 0;
#ifdef SURFACE_LOAD_MIPCONE
			lod = compute_texture_lod(tex, material.uvset_specularMap == 0 ? lod_constant0 : lod_constant1, ray_direction, surf_normal, cone_width);
#endif // SURFACE_LOAD_MIPCONE
			specularMap = tex.SampleLevel(sam, UV_specularMap, lod);
			specularMap.rgb = DEGAMMA(specularMap.rgb);
		}

		create(material, baseColor, surfaceMap, specularMap);

		emissiveColor = material.GetEmissive() * Unpack_R11G11B10_FLOAT(inst.emissive);
		if (is_emittedparticle)
		{
			emissiveColor *= baseColor.rgb * baseColor.a;
		}
		else
		{
			[branch]
			if (material.texture_emissivemap_index >= 0)
			{
				const float2 UV_emissiveMap = material.uvset_emissiveMap == 0 ? uvsets.xy : uvsets.zw;
				Texture2D tex = bindless_textures[NonUniformResourceIndex(material.texture_emissivemap_index)];
				float lod = 0;
#ifdef SURFACE_LOAD_MIPCONE
				lod = compute_texture_lod(tex, material.uvset_emissiveMap == 0 ? lod_constant0 : lod_constant1, ray_direction, surf_normal, cone_width);
#endif // SURFACE_LOAD_MIPCONE
				float4 emissiveMap = tex.SampleLevel(sam, UV_emissiveMap, lod);
				emissiveMap.rgb = DEGAMMA(emissiveMap.rgb);
				emissiveColor *= emissiveMap.rgb * emissiveMap.a;
			}
		}

		if (material.options & SHADERMATERIAL_OPTION_BIT_ADDITIVE)
		{
			emissiveColor += baseColor.rgb * baseColor.a;
		}

		transmission = material.transmission;
		if (material.texture_transmissionmap_index >= 0)
		{
			const float2 UV_transmissionMap = material.uvset_transmissionMap == 0 ? uvsets.xy : uvsets.zw;
			Texture2D tex = bindless_textures[NonUniformResourceIndex(material.texture_transmissionmap_index)];
			float lod = 0;
#ifdef SURFACE_LOAD_MIPCONE
			lod = compute_texture_lod(tex, material.uvset_transmissionMap == 0 ? lod_constant0 : lod_constant1, ray_direction, surf_normal, cone_width);
#endif // SURFACE_LOAD_MIPCONE
			transmission *= tex.SampleLevel(sam, UV_transmissionMap, lod).r;
		}

		[branch]
		if (material.IsOcclusionEnabled_Secondary() && material.texture_occlusionmap_index >= 0)
		{
			const float2 UV_occlusionMap = material.uvset_occlusionMap == 0 ? uvsets.xy : uvsets.zw;
			Texture2D tex = bindless_textures[NonUniformResourceIndex(material.texture_occlusionmap_index)];
			float lod = 0;
#ifdef SURFACE_LOAD_MIPCONE
			lod = compute_texture_lod(tex, material.uvset_occlusionMap == 0 ? lod_constant0 : lod_constant1, ray_direction, surf_normal, cone_width);
#endif // SURFACE_LOAD_MIPCONE
			occlusion *= tex.SampleLevel(sam, UV_occlusionMap, lod).r;
		}

		[branch]
		if (geometry.vb_tan >= 0 && material.texture_normalmap_index >= 0 && material.normalMapStrength > 0)
		{
			ByteAddressBuffer buf = bindless_buffers[NonUniformResourceIndex(geometry.vb_tan)];
			const float4 t0 = unpack_utangent(buf.Load(i0 * sizeof(uint)));
			const float4 t1 = unpack_utangent(buf.Load(i1 * sizeof(uint)));
			const float4 t2 = unpack_utangent(buf.Load(i2 * sizeof(uint)));
			float4 T = mad(t0, w, mad(t1, u, t2 * v)); // t0 * w + t1 * u + t2 * v
			T = T * 2 - 1;
			T.xyz = mul((float3x3)inst.transformInverseTranspose.GetMatrix(), T.xyz);
			T.xyz = normalize(T.xyz);
			const float3 B = normalize(cross(T.xyz, N) * T.w);
			const float3x3 TBN = float3x3(T.xyz, B, N);

			const float2 UV_normalMap = material.uvset_normalMap == 0 ? uvsets.xy : uvsets.zw;
			Texture2D tex = bindless_textures[NonUniformResourceIndex(material.texture_normalmap_index)];
			float lod = 0;
#ifdef SURFACE_LOAD_MIPCONE
			lod = compute_texture_lod(tex, material.uvset_normalMap == 0 ? lod_constant0 : lod_constant1, ray_direction, surf_normal, cone_width);
#endif // SURFACE_LOAD_MIPCONE
			const float3 normalMap = float3(tex.SampleLevel(sam, UV_normalMap, lod).rg, 1) * 2 - 1;
			N = normalize(lerp(N, mul(normalMap, TBN), material.normalMapStrength));
		}

		float3 pre0;
		float3 pre1;
		float3 pre2;
		[branch]
		if (geometry.vb_pre >= 0)
		{
			ByteAddressBuffer buf = bindless_buffers[NonUniformResourceIndex(geometry.vb_pre)];
			pre0 = asfloat(buf.Load3(i0 * sizeof(uint4)));
			pre1 = asfloat(buf.Load3(i1 * sizeof(uint4)));
			pre2 = asfloat(buf.Load3(i2 * sizeof(uint4)));
		}
		else
		{
			pre0 = asfloat(data0.xyz);
			pre1 = asfloat(data1.xyz);
			pre2 = asfloat(data2.xyz);
		}
		pre = mad(pre0, w, mad(pre1, u, pre2 * v)); // pre0 * w + pre1 * u + pre2 * v
		pre = mul(inst.transformPrev.GetMatrix(), float4(pre, 1)).xyz;

		sss = material.subsurfaceScattering;
		sss_inv = material.subsurfaceScattering_inv;

		update();
	}

	bool load(in PrimitiveID prim, in float2 barycentrics)
	{
		if (!preload_internal(prim))
			return false;

		bary = barycentrics;
		float u = bary.x;
		float v = bary.y;
		float w = 1 - u - v;

		float3 p0 = asfloat(data0.xyz);
		float3 p1 = asfloat(data1.xyz);
		float3 p2 = asfloat(data2.xyz);
		P = mad(p0, w, mad(p1, u, p2 * v)); // p0 * w + p1 * u + p2 * v
		P = mul(inst.transform.GetMatrix(), float4(P, 1)).xyz;

		load_internal();
		return true;
	}
	bool load(in PrimitiveID prim, in float3 worldPosition)
	{
		if (!preload_internal(prim))
			return false;

		float3 p0 = asfloat(data0.xyz);
		float3 p1 = asfloat(data1.xyz);
		float3 p2 = asfloat(data2.xyz);
		float3 P0 = mul(inst.transform.GetMatrix(), float4(p0, 1)).xyz;
		float3 P1 = mul(inst.transform.GetMatrix(), float4(p1, 1)).xyz;
		float3 P2 = mul(inst.transform.GetMatrix(), float4(p2, 1)).xyz;
		P = worldPosition;

		bary = compute_barycentrics(P, P0, P1, P2);

		load_internal();
		return true;
	}
	bool load(in PrimitiveID prim, in float3 rayOrigin, in float3 rayDirection)
	{
		if (!preload_internal(prim))
			return false;

		float3 p0 = asfloat(data0.xyz);
		float3 p1 = asfloat(data1.xyz);
		float3 p2 = asfloat(data2.xyz);
		float3 P0 = mul(inst.transform.GetMatrix(), float4(p0, 1)).xyz;
		float3 P1 = mul(inst.transform.GetMatrix(), float4(p1, 1)).xyz;
		float3 P2 = mul(inst.transform.GetMatrix(), float4(p2, 1)).xyz;

		bary = compute_barycentrics(rayOrigin, rayDirection, P0, P1, P2, hit_depth);
		P = rayOrigin + rayDirection * hit_depth;
		V = rayDirection;

		load_internal();
		return true;
	}
};

struct SurfaceToLight
{
	float3 L;		// surface to light vector (normalized)
	float3 H;		// half-vector between view vector and light vector
	float NdotL;	// cos angle between normal and light direction
	float3 NdotL_sss;	// NdotL with subsurface parameters applied
	float NdotH;	// cos angle between normal and half vector
	float LdotH;	// cos angle between light direction and half vector
	float VdotH;	// cos angle between view direction and half vector
	float3 F;		// fresnel term computed from VdotH

	// Aniso params:
	float TdotL;
	float BdotL;
	float TdotH;
	float BdotH;

	inline void create(in Surface surface, in float3 Lnormalized)
	{
		L = Lnormalized;
		H = normalize(L + surface.V);

		NdotL = dot(L, surface.N);

#ifdef BRDF_NDOTL_BIAS
		NdotL += BRDF_NDOTL_BIAS;
#endif // BRDF_NDOTL_BIAS

		NdotL_sss = (NdotL + surface.sss.rgb) * surface.sss_inv.rgb;

		NdotH = saturate(dot(surface.N, H));
		LdotH = saturate(dot(L, H));
		VdotH = saturate(dot(surface.V, H));

		F = F_Schlick(surface.f0, surface.f90, VdotH);

		TdotL = dot(surface.T, L);
		BdotL = dot(surface.B, L);
		TdotH = dot(surface.T, H);
		BdotH = dot(surface.B, H);

#ifdef BRDF_CARTOON
		// SSS is handled differently in cartoon shader:
		//	1) The diffuse wraparound is monochrome at first to avoid banding with smoothstep()
		NdotL_sss = (NdotL + surface.sss.a) * surface.sss_inv.a;

		NdotL = smoothstep(0.005, 0.05, NdotL);
		NdotL_sss = smoothstep(0.005, 0.05, NdotL_sss);
		NdotH = smoothstep(0.98, 0.99, NdotH);

		// SSS is handled differently in cartoon shader:
		//	2) The diffuse wraparound is tinted after smoothstep
		NdotL_sss = (NdotL_sss + surface.sss.rgb) * surface.sss_inv.rgb;
#endif // BRDF_CARTOON

		NdotL = saturate(NdotL);
		NdotL_sss = saturate(NdotL_sss);
	}
};


// These are the functions that will be used by shaders:

float3 BRDF_GetSpecular(in Surface surface, in SurfaceToLight surfaceToLight)
{
#ifdef BRDF_ANISOTROPIC
	float D = D_GGX_Anisotropic(surface.at, surface.ab, surfaceToLight.TdotH, surfaceToLight.BdotH, surfaceToLight.NdotH);
	float Vis = V_SmithGGXCorrelated_Anisotropic(surface.at, surface.ab, surface.TdotV, surface.BdotV,
		surfaceToLight.TdotL, surfaceToLight.BdotL, surface.NdotV, surfaceToLight.NdotL);
#else
	float D = D_GGX(surface.roughnessBRDF, surfaceToLight.NdotH, surfaceToLight.H);
	float Vis = V_SmithGGXCorrelated(surface.roughnessBRDF, surface.NdotV, surfaceToLight.NdotL);
#endif // BRDF_ANISOTROPIC

	float3 specular = D * Vis * surfaceToLight.F;

#ifdef BRDF_SHEEN
	specular *= surface.sheen.albedoScaling;
	D = D_Charlie(surface.sheen.roughnessBRDF, surfaceToLight.NdotH);
	Vis = V_Neubelt(surface.NdotV, surfaceToLight.NdotL);
	specular += D * Vis * surface.sheen.color;
#endif // BRDF_SHEEN

#ifdef BRDF_CLEARCOAT
	specular *= 1 - surface.clearcoat.F;
	float NdotH = saturate(dot(surface.clearcoat.N, surfaceToLight.H));
	D = D_GGX(surface.clearcoat.roughnessBRDF, NdotH, surfaceToLight.H);
	Vis = V_Kelemen(surfaceToLight.LdotH);
	specular += D * Vis * surface.clearcoat.F;
#endif // BRDF_CLEARCOAT

	return specular * surfaceToLight.NdotL;
}
float3 BRDF_GetDiffuse(in Surface surface, in SurfaceToLight surfaceToLight)
{
	// Note: subsurface scattering will remove Fresnel (F), because otherwise
	//	there would be artifact on backside where diffuse wraps
	float3 diffuse = (1 - lerp(surfaceToLight.F, 0, saturate(surface.sss.a)));

#ifdef BRDF_SHEEN
	diffuse *= surface.sheen.albedoScaling;
#endif // BRDF_SHEEN

#ifdef BRDF_CLEARCOAT
	diffuse *= 1 - surface.clearcoat.F;
#endif // BRDF_CLEARCOAT

	return diffuse * surfaceToLight.NdotL_sss;
}

#endif // WI_BRDF_HF
