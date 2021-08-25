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
	float4 emissiveColor;	// light emission [0 -> 1]
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
	bool receiveshadow;
	float3 facenormal;		// surface normal without normal map

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
		receiveshadow = true;
		facenormal = 0;

		sheen.color = 0;
		sheen.roughness = 0;

		clearcoat.factor = 0;
		clearcoat.roughness = 0;
		clearcoat.N = 0;
	}

	inline void create(
		in ShaderMaterial material,
		in float4 baseColor,
		in float4 surfaceMap,
		in float4 specularMap = 1
	)
	{
		init();

		if (material.options & SHADERMATERIAL_OPTION_BIT_TRANSPARENT || material.alphaTest > 0)
		{
			opacity = baseColor.a;
		}
		else
		{
			opacity = 1;
		}
		roughness = material.roughness;
		f0 = material.specularColor.rgb * specularMap.rgb * specularMap.a * material.specularColor.a;

		if (g_xFrame_Options & OPTION_BIT_FORCE_DIFFUSE_LIGHTING)
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
			float metalness = material.metalness * surfaceMap.b;
			float reflectance = material.reflectance * surfaceMap.a;
			albedo = lerp(lerp(baseColor.rgb, float3(0, 0, 0), reflectance), float3(0, 0, 0), metalness);
			f0 *= lerp(lerp(float3(0, 0, 0), float3(1, 1, 1), reflectance), baseColor.rgb, metalness);
		}

		receiveshadow = material.IsReceiveShadow();
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

	inline bool IsReceiveShadow() { return receiveshadow; }


	ShaderMeshInstance inst; 
	ShaderMeshSubset subset; 
	ShaderMaterial material;
	float2 bary;
	float3 pre;

	bool load(in PrimitiveID prim, in float2 barycentrics, in uint uid = 0)
	{
		inst = InstanceArray[prim.instanceIndex];
		if ((uid != 0 && inst.uid != uid) || inst.mesh.vb_pos_nor_wind < 0)
			return false;

		subset = bindless_subsets[NonUniformResourceIndex(inst.mesh.subsetbuffer)][prim.subsetIndex];
		material = bindless_buffers[NonUniformResourceIndex(subset.material)].Load<ShaderMaterial>(0);
		bary = barycentrics;

		uint startIndex = prim.primitiveIndex * 3 + subset.indexOffset;
		uint i0 = bindless_ib[NonUniformResourceIndex(inst.mesh.ib)][startIndex + 0];
		uint i1 = bindless_ib[NonUniformResourceIndex(inst.mesh.ib)][startIndex + 1];
		uint i2 = bindless_ib[NonUniformResourceIndex(inst.mesh.ib)][startIndex + 2];

		uint4 data0 = bindless_buffers[NonUniformResourceIndex(inst.mesh.vb_pos_nor_wind)].Load4(i0 * 16);
		uint4 data1 = bindless_buffers[NonUniformResourceIndex(inst.mesh.vb_pos_nor_wind)].Load4(i1 * 16);
		uint4 data2 = bindless_buffers[NonUniformResourceIndex(inst.mesh.vb_pos_nor_wind)].Load4(i2 * 16);
		float3 p0 = asfloat(data0.xyz);
		float3 p1 = asfloat(data1.xyz);
		float3 p2 = asfloat(data2.xyz);
		float3 n0 = unpack_unitvector(data0.w);
		float3 n1 = unpack_unitvector(data1.w);
		float3 n2 = unpack_unitvector(data2.w);

		float u = barycentrics.x;
		float v = barycentrics.y;
		float w = 1 - u - v;

		P = p0 * w + p1 * u + p2 * v;
		P = mul(inst.GetTransform(), float4(P, 1)).xyz;

		float4 uv0 = 0, uv1 = 0, uv2 = 0;
		[branch]
		if (inst.mesh.vb_uv0 >= 0)
		{
			uv0.xy = unpack_half2(bindless_buffers[NonUniformResourceIndex(inst.mesh.vb_uv0)].Load(i0 * 4));
			uv1.xy = unpack_half2(bindless_buffers[NonUniformResourceIndex(inst.mesh.vb_uv0)].Load(i1 * 4));
			uv2.xy = unpack_half2(bindless_buffers[NonUniformResourceIndex(inst.mesh.vb_uv0)].Load(i2 * 4));
		}
		[branch]
		if (inst.mesh.vb_uv1 >= 0)
		{
			uv0.zw = unpack_half2(bindless_buffers[NonUniformResourceIndex(inst.mesh.vb_uv1)].Load(i0 * 4));
			uv1.zw = unpack_half2(bindless_buffers[NonUniformResourceIndex(inst.mesh.vb_uv1)].Load(i1 * 4));
			uv2.zw = unpack_half2(bindless_buffers[NonUniformResourceIndex(inst.mesh.vb_uv1)].Load(i2 * 4));
		}
		float4 uvsets = uv0 * w + uv1 * u + uv2 * v;
		uvsets.xy = uvsets.xy * material.texMulAdd.xy + material.texMulAdd.zw;

		float4 baseColor = material.baseColor;
		[branch]
		if (material.texture_basecolormap_index >= 0)
		{
			const float2 UV_baseColorMap = material.uvset_baseColorMap == 0 ? uvsets.xy : uvsets.zw;
			float4 baseColorMap = bindless_textures[NonUniformResourceIndex(material.texture_basecolormap_index)].SampleLevel(sampler_linear_wrap, UV_baseColorMap, 0);
			if ((g_xFrame_Options & OPTION_BIT_DISABLE_ALBEDO_MAPS) == 0)
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
		if (inst.mesh.vb_col >= 0 && material.IsUsingVertexColors())
		{
			float4 c0, c1, c2;
			const uint stride_COL = 4;
			c0 = unpack_rgba(bindless_buffers[NonUniformResourceIndex(inst.mesh.vb_col)].Load(i0 * stride_COL));
			c1 = unpack_rgba(bindless_buffers[NonUniformResourceIndex(inst.mesh.vb_col)].Load(i1 * stride_COL));
			c2 = unpack_rgba(bindless_buffers[NonUniformResourceIndex(inst.mesh.vb_col)].Load(i2 * stride_COL));
			float4 vertexColor = c0 * w + c1 * u + c2 * v;
			baseColor *= vertexColor;
		}

		float4 surfaceMap = 1;
		[branch]
		if (material.texture_surfacemap_index >= 0)
		{
			const float2 UV_surfaceMap = material.uvset_surfaceMap == 0 ? uvsets.xy : uvsets.zw;
			surfaceMap = bindless_textures[NonUniformResourceIndex(material.texture_surfacemap_index)].SampleLevel(sampler_linear_wrap, UV_surfaceMap, 0);
		}

		float4 specularMap = 1;
		[branch]
		if (material.texture_specularmap_index >= 0)
		{
			const float2 UV_specularMap = material.uvset_specularMap == 0 ? uvsets.xy : uvsets.zw;
			specularMap = bindless_textures[NonUniformResourceIndex(material.texture_specularmap_index)].SampleLevel(sampler_linear_wrap, UV_specularMap, 0);
			specularMap.rgb = DEGAMMA(specularMap.rgb);
		}

		create(material, baseColor, surfaceMap, specularMap);

		emissiveColor = material.emissiveColor;
		[branch]
		if (material.texture_emissivemap_index >= 0)
		{
			const float2 UV_emissiveMap = material.uvset_emissiveMap == 0 ? uvsets.xy : uvsets.zw;
			float4 emissiveMap = bindless_textures[NonUniformResourceIndex(material.texture_emissivemap_index)].SampleLevel(sampler_linear_wrap, UV_emissiveMap, 0);
			emissiveMap.rgb = DEGAMMA(emissiveMap.rgb);
			emissiveColor *= emissiveMap;
		}

		transmission = material.transmission;
		if (material.texture_transmissionmap_index >= 0)
		{
			const float2 UV_transmissionMap = material.uvset_transmissionMap == 0 ? uvsets.xy : uvsets.zw;
			float transmissionMap = bindless_textures[NonUniformResourceIndex(material.texture_transmissionmap_index)].SampleLevel(sampler_linear_wrap, UV_transmissionMap, 0).r;
			transmission *= transmissionMap;
		}

		[branch]
		if (material.IsOcclusionEnabled_Secondary() && material.texture_occlusionmap_index >= 0)
		{
			const float2 UV_occlusionMap = material.uvset_occlusionMap == 0 ? uvsets.xy : uvsets.zw;
			occlusion *= bindless_textures[NonUniformResourceIndex(material.texture_occlusionmap_index)].SampleLevel(sampler_linear_wrap, UV_occlusionMap, 0).r;
		}

		N = n0 * w + n1 * u + n2 * v;
		N = mul((float3x3)inst.GetTransform(), N);
		N = normalize(N);
		facenormal = N;

		[branch]
		if (inst.mesh.vb_tan >= 0 && material.texture_normalmap_index >= 0 && material.normalMapStrength > 0)
		{
			float4 t0, t1, t2;
			const uint stride_TAN = 4;
			t0 = unpack_utangent(bindless_buffers[NonUniformResourceIndex(inst.mesh.vb_tan)].Load(i0 * stride_TAN));
			t1 = unpack_utangent(bindless_buffers[NonUniformResourceIndex(inst.mesh.vb_tan)].Load(i1 * stride_TAN));
			t2 = unpack_utangent(bindless_buffers[NonUniformResourceIndex(inst.mesh.vb_tan)].Load(i2 * stride_TAN));
			float4 T = t0 * w + t1 * u + t2 * v;
			T = T * 2 - 1;
			T.xyz = mul((float3x3)inst.GetTransform(), T.xyz);
			T.xyz = normalize(T.xyz);
			float3 B = normalize(cross(T.xyz, N) * T.w);
			float3x3 TBN = float3x3(T.xyz, B, N);

			const float2 UV_normalMap = material.uvset_normalMap == 0 ? uvsets.xy : uvsets.zw;
			float3 normalMap = bindless_textures[NonUniformResourceIndex(material.texture_normalmap_index)].SampleLevel(sampler_linear_wrap, UV_normalMap, 0).rgb;
			normalMap.b = normalMap.b == 0 ? 1 : normalMap.b; // fix for missing blue channel
			normalMap = normalMap * 2 - 1;
			N = normalize(lerp(N, mul(normalMap, TBN), material.normalMapStrength));
		}

		[branch]
		if (inst.mesh.vb_pre >= 0)
		{
			p0 = asfloat(bindless_buffers[inst.mesh.vb_pre].Load3(i0 * 16));
			p1 = asfloat(bindless_buffers[inst.mesh.vb_pre].Load3(i1 * 16));
			p2 = asfloat(bindless_buffers[inst.mesh.vb_pre].Load3(i2 * 16));
		}
		pre = p0 * w + p1 * u + p2 * v;
		pre = mul(inst.GetTransformPrev(), float4(pre, 1)).xyz;

		return true;
	}

	bool load(in PrimitiveID prim, in float3 P, in uint uid = 0)
	{
		inst = InstanceArray[prim.instanceIndex];
		if ((uid != 0 && inst.uid != uid) || inst.mesh.vb_pos_nor_wind < 0)
			return false;

		subset = bindless_subsets[NonUniformResourceIndex(inst.mesh.subsetbuffer)][prim.subsetIndex];
		material = bindless_buffers[NonUniformResourceIndex(subset.material)].Load<ShaderMaterial>(0);

		uint startIndex = prim.primitiveIndex * 3 + subset.indexOffset;
		uint i0 = bindless_ib[NonUniformResourceIndex(inst.mesh.ib)][startIndex + 0];
		uint i1 = bindless_ib[NonUniformResourceIndex(inst.mesh.ib)][startIndex + 1];
		uint i2 = bindless_ib[NonUniformResourceIndex(inst.mesh.ib)][startIndex + 2];

		uint4 data0 = bindless_buffers[NonUniformResourceIndex(inst.mesh.vb_pos_nor_wind)].Load4(i0 * 16);
		uint4 data1 = bindless_buffers[NonUniformResourceIndex(inst.mesh.vb_pos_nor_wind)].Load4(i1 * 16);
		uint4 data2 = bindless_buffers[NonUniformResourceIndex(inst.mesh.vb_pos_nor_wind)].Load4(i2 * 16);
		float3 p0 = asfloat(data0.xyz);
		float3 p1 = asfloat(data1.xyz);
		float3 p2 = asfloat(data2.xyz);
		float3 P0 = mul(inst.GetTransform(), float4(p0, 1)).xyz;
		float3 P1 = mul(inst.GetTransform(), float4(p1, 1)).xyz;
		float3 P2 = mul(inst.GetTransform(), float4(p2, 1)).xyz;

		float2 barycentrics = compute_barycentrics(P, P0, P1, P2);

		return load(prim, barycentrics, uid);
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

	return specular;
}
float3 BRDF_GetDiffuse(in Surface surface, in SurfaceToLight surfaceToLight)
{
	// Note: subsurface scattering will remove Fresnel (F), because otherwise
	//	there would be artifact on backside where diffuse wraps
	float3 diffuse = (1 - lerp(surfaceToLight.F, 0, saturate(surface.sss.a))) / PI;

#ifdef BRDF_SHEEN
	diffuse *= surface.sheen.albedoScaling;
#endif // BRDF_SHEEN

#ifdef BRDF_CLEARCOAT
	diffuse *= 1 - surface.clearcoat.F;
#endif // BRDF_CLEARCOAT

	return diffuse;
}

#endif // WI_BRDF_HF
