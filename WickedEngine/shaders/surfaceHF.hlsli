#ifndef WI_SURFACE_HF
#define WI_SURFACE_HF
#include "globals.hlsli"

// hard coded value for surfaces with simplified lighting:
//	occlusion = 1
//	roughness = 1
//	metalness = 0
//	reflectance = 0
static const half4 surfacemap_simple = half4(1, 1, 0, 0);
#define min_roughness 0.045

half3 F_Schlick(const half3 f0, half VoH)
{
	// Schlick 1994, "An Inexpensive BRDF Model for Physically-Based Rendering"
	half f90 = saturate(50.0 * dot(f0, 0.33)); // reflectance at grazing angle
	return f0 + (f90 - f0) * pow5(1.0 - VoH);
}

// https://www.unrealengine.com/en-US/blog/physically-based-shading-on-mobile
half3 EnvBRDFApprox(half3 SpecularColor, half Roughness, half NoV)
{
	const half4 c0 = { -1, -0.0275, -0.572, 0.022 };
	const half4 c1 = { 1, 0.0425, 1.04, -0.04 };
	half4 r = Roughness * c0 + c1;
	half a004 = min(r.x * r.x, exp2(-9.28 * NoV)) * r.x + r.y;
	half2 AB = half2(-1.04, 1.04) * a004 + r.zw;
	return SpecularColor * AB.x + AB.y;
}

struct SheenSurface
{
	half3 color;
	half roughness;

	// computed values:
	half DFG;
	half albedoScaling;
};

struct ClearcoatSurface
{
	half factor;
	half roughness;
	float3 N;

	// computed values:
	half3 R;
	half3 F;
};

struct AnisotropicSurface
{
	half2 direction;
	half strength;
	half3 T;

	// computed values:
	float at;
	float ab;
	half3 B;
	half TdotV;
	half BdotV;
};

struct Surface
{
	// Fill these yourself:
	float3 P;				// world space position
	float3 N;				// world space normal
	float3 V;				// world space view vector

	half4 baseColor;
	half3 albedo;			// diffuse light absorbtion value (rgb)
	half3 f0;				// fresnel value (rgb) (reflectance at incidence angle, also known as specular color)
	half roughness;			// roughness: [0:smooth -> 1:rough] (perceptual)
	half occlusion;			// occlusion [0 -> 1]
	half opacity;			// opacity for blending operation [0 -> 1]
	half3 emissiveColor;	// light emission [0 -> 1]
	half4 refraction;		// refraction color (rgb), refraction amount (a)
	half transmission;		// transmission factor
	min16uint2 pixel;		// pixel coordinate (used for randomization effects)
	float2 screenUV;		// pixel coordinate in UV space [0 -> 1] (used for randomization effects)
	half4 T;				// tangent
	half3 B;				// bitangent
	half4 sss;				// subsurface scattering color * amount
	half4 sss_inv;			// 1 / (1 + sss)
	uint layerMask;			// the engine-side layer mask
	half3 facenormal;		// surface normal without normal map
	uint uid_validate;
	float hit_depth;
	half3 gi;
	half3 bumpColor;
	half3 ssgi;
	half3 extinction;

	// It's good to use bools instead of flags in shaders, because bools of all lanes can be combined into one SGPR
	bool receiveshadow;
	bool is_backface;
	bool gi_applied;

	// These will be computed when calling Update():
	half NdotV;				// cos(angle between normal and view vector)
	half3 R;				// reflection vector
	half3 F;				// fresnel term computed from NdotV

#ifdef SURFACE_LOAD_MIPCONE
	RayCone raycone;
#endif // SURFACE_LOAD_MIPCONE

#ifdef SHEEN
	SheenSurface sheen;
#endif // SHEEN

#ifdef CLEARCOAT
	ClearcoatSurface clearcoat;
#endif // CLEARCOAT

#ifdef ANISOTROPIC
	AnisotropicSurface aniso;
#endif // ANISOTROPIC

	inline void init()
	{
		P = 0;
		V = 0;
		N = 0;
		baseColor = 1;
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
		sss = 0;
		sss_inv = 1;
		layerMask = ~0;
		facenormal = 0;
		gi = 0;
		bumpColor = 0;
		ssgi = 0;
		extinction = 0;
		
		receiveshadow = true;
		is_backface = false;
		gi_applied = false;

		uid_validate = 0;
		hit_depth = 0;

#ifdef SURFACE_LOAD_MIPCONE
		raycone = (RayCone)0;
#endif // SURFACE_LOAD_MIPCONE

#ifdef SHEEN
		sheen.color = 0;
		sheen.roughness = 0;
#endif // SHEEN

#ifdef CLEARCOAT
		clearcoat.factor = 0;
		clearcoat.roughness = 0;
		clearcoat.N = 0;
#endif // CLEARCOAT

#ifdef ANISOTROPIC
		aniso.strength = 0;
		aniso.direction = float2(1, 0);
		aniso.T = 0;
#endif // ANISOTROPIC
	}

	inline void create(in ShaderMaterial material)
	{
		sss = material.GetSSS();
		sss_inv = material.GetSSSInverse();
		SetReceiveShadow(material.IsReceiveShadow());
	}

	inline void create(
		in ShaderMaterial material,
		in half4 _baseColor,
		in half4 surfaceMap,
		in half4 specularMap = 1
	)
	{
		baseColor = _baseColor;
		if (material.IsTransparent() || material.GetAlphaTest() > 0)
		{
			opacity = baseColor.a;
		}
		else
		{
			opacity = 1;
		}
		f0 = material.GetSpecular() * specularMap.rgb * specularMap.a;

#ifndef ENVMAPRENDERING
		if (GetFrame().options & OPTION_BIT_FORCE_DIFFUSE_LIGHTING)
#endif // ENVMAPRENDERING
		{
			f0 = surfaceMap.b = surfaceMap.a = 0;
		}

		[branch]
		if (material.IsUsingSpecularGlossinessWorkflow())
		{
			// Specular-glossiness workflow:
			roughness = material.GetRoughness() * saturate(1 - surfaceMap.a);
			f0 *= surfaceMap.rgb;
			albedo = baseColor.rgb;
		}
		else
		{
			// Metallic-roughness workflow:
			if (material.IsOcclusionEnabled_Primary())
			{
				occlusion *= surfaceMap.r;
			}
			roughness = surfaceMap.g;
			const half metalness = surfaceMap.b;
			const half reflectance = surfaceMap.a;
			albedo = baseColor.rgb * (1 - max(reflectance, metalness));
			f0 *= lerp(reflectance.xxx, baseColor.rgb, metalness);
		}

#ifdef UNLIT
		albedo = baseColor.rgb;
#endif // UNLIT

		create(material);
	}
	
	inline void update()
	{
		// the basic roughness must be between [0,1], do not clamp it here for BRDF,
		//	because it is also used for other effects (eg. envmaps, reflections, etc.)
		//	BRDF roughness (squared) will have the clamping just before it is computed
		roughness = saturate(roughness);

#ifdef SHEEN
		sheen.roughness = saturate(sheen.roughness);
#endif // SHEEN

#ifdef CLEARCOAT
		clearcoat.roughness = saturate(clearcoat.roughness);
#endif // CLEARCOAT

		NdotV = saturate(dot(N, V) + 1e-5);
		
#ifdef CARTOON
		F = F_Schlick(f0, NdotV);
#else
		F = EnvBRDFApprox(f0, roughness, NdotV);
#endif // CARTOON

		R = -reflect(V, N);

#ifdef SHEEN
		// Sheen energy compensation: https://dassaultsystemes-technology.github.io/EnterprisePBRShadingModel/spec-2021x.md.html#figure_energy-compensation-sheen-e
		sheen.DFG = texture_sheenlut.SampleLevel(sampler_linear_clamp, float2(NdotV, sheen.roughness), 0).r;
		sheen.albedoScaling = 1.0 - max3(sheen.color) * sheen.DFG;
#endif // SHEEN

#ifdef CLEARCOAT
		float clearcoatNdotV = saturate(dot(clearcoat.N, V) + 1e-5);
#ifdef CARTOON
		clearcoat.F = F_Schlick(f0, clearcoatNdotV);
#else
		clearcoat.F = EnvBRDFApprox(f0, clearcoat.roughness, clearcoatNdotV);
#endif // CARTOON
		clearcoat.F *= clearcoat.factor;
		clearcoat.R = -reflect(V, clearcoat.N);
#endif // CLEARCOAT

		B = cross(T.xyz, N) * T.w; // Compute bitangent again after normal mapping

#ifdef ANISOTROPIC
		aniso.B = cross(N, aniso.T);
		aniso.TdotV = dot(aniso.T.xyz, V);
		aniso.BdotV = dot(aniso.B, V);
		half roughnessBRDF = sqr(clamp(roughness, min_roughness, 1));
		aniso.at = max(0, roughnessBRDF * (1 + aniso.strength));
		aniso.ab = max(0, roughnessBRDF * (1 - aniso.strength));
#endif // ANISOTROPIC

#ifdef CARTOON
		F = smoothstep(0.1, 0.5, F);
#endif // CARTOON
		
#ifndef ENVMAPRENDERING
		if (GetFrame().options & OPTION_BIT_FORCE_DIFFUSE_LIGHTING)
#endif // ENVMAPRENDERING
		{
			F = 0;
		}
	}

	inline bool IsReceiveShadow() { return receiveshadow; }
	inline bool IsBackface() { return is_backface; }
	inline bool IsGIApplied() { return gi_applied; }
	
	inline void SetReceiveShadow(bool value) { receiveshadow = value; }
	inline void SetBackface(bool value) { is_backface = value; }
	inline void SetGIApplied(bool value) { gi_applied = value; }


	ShaderMeshInstance inst;
	ShaderGeometry geometry;
	ShaderMaterial material;
	float2 bary;
#ifdef SURFACE_LOAD_QUAD_DERIVATIVES
	float2 bary_quad_x;
	float2 bary_quad_y;
	float3 P_dx;
	float3 P_dy;
#endif // SURFACE_LOAD_QUAD_DERIVATIVES
	uint i0;
	uint i1;
	uint i2;
	float4 data0;
	float4 data1;
	float4 data2;
	float3 pre;

	bool preload_internal(PrimitiveID prim)
	{
		inst = load_instance(prim.instanceIndex);
		if (uid_validate != 0 && inst.uid != uid_validate)
			return false;

		geometry = load_geometry(inst.geometryOffset + prim.subsetIndex);
		if (geometry.vb_pos_wind < 0)
			return false;

		material = load_material(geometry.materialIndex);
		create(material);

		layerMask = material.layerMask & inst.layerMask;

		const uint startIndex = prim.primitiveIndex * 3 + geometry.indexOffset;
		Buffer<uint> indexBuffer = bindless_buffers_uint[NonUniformResourceIndex(geometry.ib)];
		i0 = indexBuffer[startIndex + 0];
		i1 = indexBuffer[startIndex + 1];
		i2 = indexBuffer[startIndex + 2];

		Buffer<float4> buf = bindless_buffers_float4[NonUniformResourceIndex(geometry.vb_pos_wind)];
		data0 = buf[i0];
		data1 = buf[i1];
		data2 = buf[i2];

		return true;
	}
	void load_internal(uint flatTileIndex = 0)
	{
		SamplerState sam = bindless_samplers[material.sampler_descriptor];

		const bool is_hairparticle = geometry.flags & SHADERMESH_FLAG_HAIRPARTICLE;
		const bool is_emittedparticle = geometry.flags & SHADERMESH_FLAG_EMITTEDPARTICLE;
		const bool simple_lighting = is_hairparticle || is_emittedparticle;

		half3 Nunnormalized = 0;
		
		[branch]
		if (geometry.vb_nor >= 0)
		{
			Buffer<float4> buf = bindless_buffers_float4[NonUniformResourceIndex(geometry.vb_nor)];
			half3 n0 = rotate_vector(buf[i0].xyz, (half4)inst.quaternion);
			half3 n1 = rotate_vector(buf[i1].xyz, (half4)inst.quaternion);
			half3 n2 = rotate_vector(buf[i2].xyz, (half4)inst.quaternion);
			n0 = any(n0) ? normalize(n0) : 0;
			n1 = any(n1) ? normalize(n1) : 0;
			n2 = any(n2) ? normalize(n2) : 0;
			N = attribute_at_bary(n0, n1, n2, bary);
		}
		
		if (is_backface && !is_hairparticle && !is_emittedparticle)
		{
			N = -N;
		}
		Nunnormalized = N; // after normalized-interpolated at barycentric, this is unnormalized!
		N = normalize(N);
		facenormal = N;

#ifdef SURFACE_LOAD_MIPCONE
		float3 p0 = data0.xyz;
		float3 p1 = data1.xyz;
		float3 p2 = data2.xyz;
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
		float4 uvsets_dx = 0;
		float4 uvsets_dy = 0;
		[branch]
		if (geometry.vb_uvs >= 0)
		{
			Buffer<float4> buf = bindless_buffers_float4[NonUniformResourceIndex(geometry.vb_uvs)];
			float4 uv0 = lerp(geometry.uv_range_min.xyxy, geometry.uv_range_max.xyxy, buf[i0]);
			float4 uv1 = lerp(geometry.uv_range_min.xyxy, geometry.uv_range_max.xyxy, buf[i1]);
			float4 uv2 = lerp(geometry.uv_range_min.xyxy, geometry.uv_range_max.xyxy, buf[i2]);
			// all three must be transformed, to have correct derivatives (not enough to only transform final uvsets):
			uv0.xy = mad(uv0.xy, material.texMulAdd.xy, material.texMulAdd.zw);
			uv1.xy = mad(uv1.xy, material.texMulAdd.xy, material.texMulAdd.zw);
			uv2.xy = mad(uv2.xy, material.texMulAdd.xy, material.texMulAdd.zw);
			uvsets = attribute_at_bary(uv0, uv1, uv2, bary);

#ifdef SURFACE_LOAD_MIPCONE
			lod_constant0 = 0.5 * log2(twice_uv_area(uv0.xy, uv1.xy, uv2.xy) * triangle_constant);
			lod_constant1 = 0.5 * log2(twice_uv_area(uv0.zw, uv1.zw, uv2.zw) * triangle_constant);

			const float lod_uvset0 = compute_texture_lod(65536, 65536, lod_constant0, ray_direction, surf_normal, cone_width);
			const float lod_uvset1 = compute_texture_lod(65536, 65536, lod_constant1, ray_direction, surf_normal, cone_width);
			const uint resolution0 = 65536u >> uint(max(0, lod_uvset0));
			const uint resolution1 = 65536u >> uint(max(0, lod_uvset1));
			write_mipmap_feedback(geometry.materialIndex, resolution0, resolution1);
#endif // SURFACE_LOAD_MIPCONE

#ifdef SURFACE_LOAD_QUAD_DERIVATIVES
			uvsets_dx = uvsets - attribute_at_bary(uv0, uv1, uv2, bary_quad_x);
			uvsets_dy = uvsets - attribute_at_bary(uv0, uv1, uv2, bary_quad_y);
			write_mipmap_feedback(geometry.materialIndex, uvsets_dx, uvsets_dy);
#endif // SURFACE_LOAD_QUAD_DERIVATIVES
		}

		half3x3 TBN = half3x3(1,0,0, 0,1,0, 0,0,1);

		[branch]
		if (geometry.vb_tan >= 0)
		{
			Buffer<float4> buf = bindless_buffers_float4[NonUniformResourceIndex(geometry.vb_tan)];
			half4 t0 = buf[i0];
			half4 t1 = buf[i1];
			half4 t2 = buf[i2];
			t0.xyz = rotate_vector(t0.xyz, (half4)inst.quaternion);
			t1.xyz = rotate_vector(t1.xyz, (half4)inst.quaternion);
			t2.xyz = rotate_vector(t2.xyz, (half4)inst.quaternion);
			t0.xyz = any(t0.xyz) ? normalize(t0.xyz) : 0;
			t1.xyz = any(t1.xyz) ? normalize(t1.xyz) : 0;
			t2.xyz = any(t2.xyz) ? normalize(t2.xyz) : 0;
			T = attribute_at_bary(t0, t1, t2, bary);
			if (is_backface)
			{
				T = -T;
			}
			T.w = T.w < 0 ? -1 : 1;
			half3 bitangent = cross(T.xyz, Nunnormalized) * T.w;
			TBN = half3x3(T.xyz, bitangent, Nunnormalized); // unnormalized TBN! http://www.mikktspace.com/
			
			T.xyz = normalize(T.xyz);

#ifdef PARALLAXOCCLUSIONMAPPING
			[branch]
			if (material.textures[DISPLACEMENTMAP].IsValid())
			{
				const float2 uv = material.textures[DISPLACEMENTMAP].GetUVSet() == 0 ? uvsets.xy : uvsets.zw;
				const float2 uv_dx = material.textures[DISPLACEMENTMAP].GetUVSet() == 0 ? uvsets_dx.xy : uvsets_dx.zw;
				const float2 uv_dy = material.textures[DISPLACEMENTMAP].GetUVSet() == 0 ? uvsets_dy.xy : uvsets_dy.zw;
				Texture2D tex = bindless_textures[NonUniformResourceIndex(material.textures[DISPLACEMENTMAP].texture_descriptor)];
				ParallaxOcclusionMapping_Impl(
					uvsets,
					V,
					TBN,
					material.GetParallaxOcclusionMapping(),
					tex,
					uv,
					uv_dx,
					uv_dy
				);
			}
#endif // PARALLAXOCCLUSIONMAPPING

			// Normal mapping:
			[branch]
			if (geometry.vb_tan >= 0 && material.textures[NORMALMAP].IsValid())
			{
#ifdef SURFACE_LOAD_QUAD_DERIVATIVES
				bumpColor = half3(material.textures[NORMALMAP].SampleGrad(sam, uvsets, uvsets_dx, uvsets_dy).rg, 1);
#else
				float lod = 0;
#ifdef SURFACE_LOAD_MIPCONE
				lod = compute_texture_lod(material.textures[NORMALMAP].GetTexture(), material.textures[NORMALMAP].GetUVSet() == 0 ? lod_constant0 : lod_constant1, ray_direction, surf_normal, cone_width);
#endif // SURFACE_LOAD_MIPCONE
				bumpColor = half3(material.textures[NORMALMAP].SampleLevel(sam, uvsets, lod).rg, 1);
#endif // SURFACE_LOAD_QUAD_DERIVATIVES
				bumpColor = bumpColor * 2 - 1;
				bumpColor.rg *= material.GetNormalMapStrength();
			}


#ifdef ANISOTROPIC
			aniso.strength = material.GetAnisotropy();
			aniso.direction = half2(material.GetAnisotropyCos(), material.GetAnisotropySin());

			[branch]
			if (material.textures[ANISOTROPYMAP].IsValid())
			{
#ifdef SURFACE_LOAD_QUAD_DERIVATIVES
				half2 anisotropyTexture = material.textures[ANISOTROPYMAP].SampleGrad(sam, uvsets, uvsets_dx, uvsets_dy).rg * 2 - 1;
#else
				float lod = 0;
#ifdef SURFACE_LOAD_MIPCONE
				lod = compute_texture_lod(material.textures[ANISOTROPYMAP].GetTexture(), material.textures[ANISOTROPYMAP].GetUVSet() == 0 ? lod_constant0 : lod_constant1, ray_direction, surf_normal, cone_width);
#endif // SURFACE_LOAD_MIPCONE
				half2 anisotropyTexture = material.textures[ANISOTROPYMAP].SampleLevel(sam, uvsets, lod).rg * 2 - 1;
#endif // SURFACE_LOAD_QUAD_DERIVATIVES

				aniso.strength *= length(anisotropyTexture);
				aniso.direction = mul(half2x2(aniso.direction.x, aniso.direction.y, -aniso.direction.y, aniso.direction.x), normalize(anisotropyTexture));
			}

			aniso.T = normalize(mul(TBN, half3(aniso.direction, 0)));

#endif // ANISOTROPIC

		}

		baseColor = is_emittedparticle ? 1 : material.GetBaseColor();
		baseColor *= inst.GetColor();
		[branch]
		if (material.textures[BASECOLORMAP].IsValid())
		{
#ifdef SURFACE_LOAD_QUAD_DERIVATIVES
			half4 baseColorMap = material.textures[BASECOLORMAP].SampleGrad(sam, uvsets, uvsets_dx, uvsets_dy);
#else
			float lod = 0;
#ifdef SURFACE_LOAD_MIPCONE
			lod = compute_texture_lod(material.textures[BASECOLORMAP].GetTexture(), material.textures[BASECOLORMAP].GetUVSet() == 0 ? lod_constant0 : lod_constant1, ray_direction, surf_normal, cone_width);
#endif // SURFACE_LOAD_MIPCONE
			half4 baseColorMap = material.textures[BASECOLORMAP].SampleLevel(sam, uvsets, lod);
#endif // SURFACE_LOAD_QUAD_DERIVATIVES
			if ((GetFrame().options & OPTION_BIT_DISABLE_ALBEDO_MAPS) == 0)
			{
				baseColor *= baseColorMap;
			}
			else
			{
				baseColor.a *= baseColorMap.a;
			}
		}
		
		[branch]
		if (material.textures[TRANSPARENCYMAP].IsValid())
		{
#ifdef SURFACE_LOAD_QUAD_DERIVATIVES
			baseColor.a *= material.textures[TRANSPARENCYMAP].SampleGrad(sam, uvsets, uvsets_dx, uvsets_dy).r;
#else
			float lod = 0;
#ifdef SURFACE_LOAD_MIPCONE
			lod = compute_texture_lod(material.textures[TRANSPARENCYMAP].GetTexture(), material.textures[TRANSPARENCYMAP].GetUVSet() == 0 ? lod_constant0 : lod_constant1, ray_direction, surf_normal, cone_width);
#endif // SURFACE_LOAD_MIPCONE
			baseColor.a *= material.textures[TRANSPARENCYMAP].SampleLevel(sam, uvsets, lod).r;
#endif // SURFACE_LOAD_QUAD_DERIVATIVES
		}

		[branch]
		if (geometry.vb_col >= 0 && material.IsUsingVertexColors())
		{
			Buffer<float4> buf = bindless_buffers_float4[NonUniformResourceIndex(geometry.vb_col)];
			const half4 c0 = buf[i0];
			const half4 c1 = buf[i1];
			const half4 c2 = buf[i2];
			half4 vertexColor = attribute_at_bary(c0, c1, c2, bary);
			baseColor *= vertexColor;
		}

		[branch]
		if (inst.vb_ao >= 0 && material.IsUsingVertexAO())
		{
			Buffer<float> buf = bindless_buffers_float[NonUniformResourceIndex(inst.vb_ao)];
			const half ao0 = buf[i0];
			const half ao1 = buf[i1];
			const half ao2 = buf[i2];
			half ao = attribute_at_bary(ao0, ao1, ao2, bary);
			occlusion = ao;
		}

		[branch]
		if (inst.lightmap >= 0 && geometry.vb_atl >= 0)
		{
			Buffer<float2> buf = bindless_buffers_float2[NonUniformResourceIndex(geometry.vb_atl)];
			const float2 a0 = buf[i0];
			const float2 a1 = buf[i1];
			const float2 a2 = buf[i2];
			float2 atlas = attribute_at_bary(a0, a1, a2, bary);

			Texture2D tex = bindless_textures[NonUniformResourceIndex(inst.lightmap)];
			gi = tex.SampleLevel(sampler_linear_clamp, atlas, 0).rgb;
			SetGIApplied(true);
		}

		half4 surfaceMap = 1;
		[branch]
		if (material.textures[SURFACEMAP].IsValid() && !simple_lighting)
		{
#ifdef SURFACE_LOAD_QUAD_DERIVATIVES
			surfaceMap = material.textures[SURFACEMAP].SampleGrad(sam, uvsets, uvsets_dx, uvsets_dy);
#else
			float lod = 0;
#ifdef SURFACE_LOAD_MIPCONE
			lod = compute_texture_lod(material.textures[SURFACEMAP].GetTexture(), material.textures[SURFACEMAP].GetUVSet() == 0 ? lod_constant0 : lod_constant1, ray_direction, surf_normal, cone_width);
#endif // SURFACE_LOAD_MIPCONE
			surfaceMap = material.textures[SURFACEMAP].SampleLevel(sam, uvsets, lod);
#endif // SURFACE_LOAD_QUAD_DERIVATIVES
		}

		[branch]
		if (!material.IsUsingSpecularGlossinessWorkflow())
		{
			// Premultiply these before evaluating decals:
			surfaceMap.g *= material.GetRoughness();
			surfaceMap.b *= material.GetMetalness();
			surfaceMap.a *= material.GetReflectance();
		}

		if (simple_lighting)
		{
			surfaceMap = surfacemap_simple;
		}

		emissiveColor = material.GetEmissive() * inst.GetEmissive();
		if (is_emittedparticle)
		{
			emissiveColor *= baseColor.rgb * baseColor.a;
		}
		else
		{
			[branch]
			if (material.textures[EMISSIVEMAP].IsValid())
			{
#ifdef SURFACE_LOAD_QUAD_DERIVATIVES
				half4 emissiveMap = material.textures[EMISSIVEMAP].SampleGrad(sam, uvsets, uvsets_dx, uvsets_dy);
#else
				float lod = 0;
#ifdef SURFACE_LOAD_MIPCONE
				lod = compute_texture_lod(material.textures[EMISSIVEMAP].GetTexture(), material.textures[EMISSIVEMAP].GetUVSet() == 0 ? lod_constant0 : lod_constant1, ray_direction, surf_normal, cone_width);
#endif // SURFACE_LOAD_MIPCONE
				half4 emissiveMap = material.textures[EMISSIVEMAP].SampleLevel(sam, uvsets, lod);
#endif // SURFACE_LOAD_QUAD_DERIVATIVES
				emissiveColor *= emissiveMap.rgb * emissiveMap.a;
			}
		}

		if (material.IsAdditive())
		{
			emissiveColor += baseColor.rgb * baseColor.a;
		}

#ifdef SURFACE_LOAD_QUAD_DERIVATIVES
#ifdef TERRAINBLENDED
		[branch]
		if (material.GetTerrainBlendRcp() > 0)
		{
			// Blend object into terrain material:
			ShaderTerrain terrain = GetScene().terrain;
			[branch]
			if(terrain.chunk_buffer >= 0)
			{
				int2 chunk_coord = floor((P.xz - terrain.center_chunk_pos.xz) / terrain.chunk_size);
				if(chunk_coord.x >= -terrain.chunk_buffer_range && chunk_coord.x <= terrain.chunk_buffer_range && chunk_coord.y >= -terrain.chunk_buffer_range && chunk_coord.y <= terrain.chunk_buffer_range)
				{
					uint chunk_idx = flatten2D(chunk_coord + terrain.chunk_buffer_range, terrain.chunk_buffer_range * 2 + 1);
					ShaderTerrainChunk chunk = bindless_structured_terrain_chunks[terrain.chunk_buffer][chunk_idx];
				
					[branch]
					if(chunk.heightmap >= 0)
					{
						Texture2D terrain_heightmap = bindless_textures[NonUniformResourceIndex(chunk.heightmap)];
						float2 chunk_min = terrain.center_chunk_pos.xz + chunk_coord * terrain.chunk_size;
						float2 chunk_max = terrain.center_chunk_pos.xz + terrain.chunk_size + chunk_coord * terrain.chunk_size;
						float2 terrain_uv = saturate(inverse_lerp(chunk_min, chunk_max, P.xz));
						float terrain_height0 = terrain_heightmap.SampleLevel(sampler_linear_clamp, terrain_uv, 0).r;
						float terrain_height1 = terrain_heightmap.SampleLevel(sampler_linear_clamp, terrain_uv, 0, int2(1, 0)).r;
						float terrain_height2 = terrain_heightmap.SampleLevel(sampler_linear_clamp, terrain_uv, 0, int2(0, 1)).r;
						float3 P0 = float3(0, terrain_height0, 0); 
						float3 P1 = float3(1, terrain_height1, 0); 
						float3 P2 = float3(0, terrain_height2, 1);
						float3 terrain_normal = normalize(cross(P2 - P0, P1 - P0));
						float terrain_height = lerp(terrain.min_height, terrain.max_height, terrain_height0);
						float object_height = P.y;
						float diff = (object_height - terrain_height) * material.GetTerrainBlendRcp();
						float blend = 1 - pow(saturate(diff), 2);
						//blend *= lerp(1, saturate((noise_gradient_3D(P * 2) * 0.5 + 0.5) * 2), saturate(diff));
						//terrain_uv = lerp(saturate(inverse_lerp(chunk_min, chunk_max, P.xz - N.xz * diff)), terrain_uv, saturate(N.y)); // uv stretching improvement: stretch in normal direction if normal gets horizontal
						ShaderMaterial terrain_material = load_material(chunk.materialID);
						terrain_uv = mad(terrain_uv, terrain_material.texMulAdd.xy, terrain_material.texMulAdd.zw);
						float2 terrain_uv_dx = terrain_uv - saturate(inverse_lerp(chunk_min, chunk_max, (P - P_dx).xz));
						float2 terrain_uv_dy = terrain_uv - saturate(inverse_lerp(chunk_min, chunk_max, (P - P_dy).xz));
						float4 terrain_baseColor = terrain_material.textures[BASECOLORMAP].SampleGrad(sam, terrain_uv.xyxy, terrain_uv_dx.xyxy, terrain_uv_dy.xyxy);
						float4 terrain_bumpColor = terrain_material.textures[NORMALMAP].SampleGrad(sam, terrain_uv.xyxy, terrain_uv_dx.xyxy, terrain_uv_dy.xyxy);
						float4 terrain_surfaceMap = terrain_material.textures[SURFACEMAP].SampleGrad(sam, terrain_uv.xyxy, terrain_uv_dx.xyxy, terrain_uv_dy.xyxy);
						float3 terrain_emissiveMap = terrain_material.textures[EMISSIVEMAP].SampleGrad(sam, terrain_uv.xyxy, terrain_uv_dx.xyxy, terrain_uv_dy.xyxy).rgb;
						baseColor = lerp(baseColor, terrain_baseColor, blend);
						bumpColor = lerp(bumpColor, terrain_bumpColor.rgb * 2 - 1, blend);
						surfaceMap = lerp(surfaceMap, terrain_surfaceMap, blend);
						emissiveColor += terrain_emissiveMap * terrain_material.GetEmissive() * blend;
						Nunnormalized = lerp(Nunnormalized, terrain_normal, blend);
						TBN[2] = Nunnormalized;
						N = normalize(Nunnormalized);
					}
				}
			}
		}
#endif // TERRAINBLENDED
#endif // SURFACE_LOAD_QUAD_DERIVATIVES

#ifdef SURFACE_LOAD_QUAD_DERIVATIVES
		// I need to copy the decal code here because include resolve issues:
#ifndef DISABLE_DECALS
		[branch]
		if (!simple_lighting && GetFrame().decalarray_count > 0)
		{
			// decals are enabled, loop through them first:
			half4 decalAccumulation = 0;
			half4 decalBumpAccumulation = 0;
			half4 decalSurfaceAccumulation = 0;
			half decalSurfaceAccumulationAlpha = 0;

			// Loop through decal buckets in the tile:
			const uint first_item = GetFrame().decalarray_offset;
			const uint last_item = first_item + GetFrame().decalarray_count - 1;
			const uint first_bucket = first_item / 32;
			const uint last_bucket = min(last_item / 32, max(0, SHADER_ENTITY_TILE_BUCKET_COUNT - 1));
			[loop]
			for (uint bucket = first_bucket; bucket <= last_bucket; ++bucket)
			{
				uint bucket_bits = load_entitytile(flatTileIndex + bucket);

#ifndef ENTITY_TILE_UNIFORM
				// This is the wave scalarizer from Improved Culling - Siggraph 2017 [Drobot]:
				bucket_bits = WaveReadLaneFirst(WaveActiveBitOr(bucket_bits));
#endif // ENTITY_TILE_UNIFORM

				[loop]
				while (WaveActiveAnyTrue(bucket_bits != 0 && decalAccumulation.a < 1 && decalBumpAccumulation.a < 1 && decalSurfaceAccumulationAlpha < 1))
				{
					// Retrieve global entity index from local bucket, then remove bit from local bucket:
					const uint bucket_bit_index = firstbitlow(bucket_bits);
					const uint entity_index = bucket * 32 + bucket_bit_index;
					bucket_bits ^= 1u << bucket_bit_index;

					[branch]
					if (entity_index >= first_item && entity_index <= last_item)
					{
						ShaderEntity decal = load_entity(entity_index);

						float4x4 decalProjection = load_entitymatrix(decal.GetMatrixIndex());
						const int decalTexture = asint(decalProjection[3][0]);
						const int decalNormal = asint(decalProjection[3][1]);
						const int decalSurfacemap = asint(decalProjection[3][2]);
						const int decalDisplacementmap = asint(decalProjection[3][3]);
						decalProjection[3] = float4(0, 0, 0, 1);
						
						// under here will be VGPR!
						if ((decal.layerMask & layerMask) == 0)
							continue;
						const float3 clipSpacePos = mul(decalProjection, float4(P, 1)).xyz;
						float3 uvw = clipspace_to_uv(clipSpacePos.xyz);
						[branch]
						if (is_saturated(uvw))
						{
							uvw.xy = mad(uvw.xy, decal.shadowAtlasMulAdd.xy, decal.shadowAtlasMulAdd.zw);
							// mipmapping needs to be performed by hand:
							const float2 decalDX = mul(P_dx, (float3x3)decalProjection).xy;
							const float2 decalDY = mul(P_dy, (float3x3)decalProjection).xy;
							half4 decalColor = decal.GetColor();
							// blend out if close to cube Z:
							const half edgeBlend = 1 - pow(saturate(abs(clipSpacePos.z)), 8);
							const half slopeBlend = decal.GetConeAngleCos() > 0 ? pow(saturate(dot(N, decal.GetDirection())), decal.GetConeAngleCos()) : 1;
							decalColor.a *= edgeBlend * slopeBlend;
							[branch]
							if (decalDisplacementmap >= 0)
							{
								const half3 t = (half3)get_right(decalProjection);
								const half3 b = -(half3)get_up(decalProjection);
								const half3 n = (half3)N;
								const half3x3 tbn = half3x3(t, b, n);
								float4 inoutuv = uvw.xyxy;
								ParallaxOcclusionMapping_Impl(
									inoutuv,
									V,
									tbn,
									decal.GetLength(),
									bindless_textures[decalDisplacementmap],
									uvw.xy,
									decalDX,
									decalDY,
									sampler_linear_clamp
								);
								uvw.xy = saturate(inoutuv.xy);
							}
							[branch]
							if (decalTexture >= 0)
							{
								decalColor *= (half4)bindless_textures[decalTexture].SampleGrad(sam, uvw.xy, decalDX, decalDY);
								if ((decal.GetFlags() & ENTITY_FLAG_DECAL_BASECOLOR_ONLY_ALPHA) == 0)
								{
									// perform manual blending of decals:
									//  NOTE: they are sorted top-to-bottom, but blending is performed bottom-to-top
									decalAccumulation.rgb = mad(1 - decalAccumulation.a, decalColor.a * decalColor.rgb, decalAccumulation.rgb);
									decalAccumulation.a = mad(1 - decalColor.a, decalAccumulation.a, decalColor.a);
								}
							}
							[branch]
							if (decalNormal >= 0)
							{
								half3 decalBumpColor = half3((half2)bindless_textures[decalNormal].SampleGrad(sam, uvw.xy, decalDX, decalDY).rg, 1);
								decalBumpColor = decalBumpColor * 2 - 1;
								decalBumpColor.rg *= decal.GetAngleScale();
								decalBumpAccumulation.rgb = mad(1 - decalBumpAccumulation.a, decalColor.a * decalBumpColor.rgb, decalBumpAccumulation.rgb);
								decalBumpAccumulation.a = mad(1 - decalColor.a, decalBumpAccumulation.a, decalColor.a);
							}
							[branch]
							if (decalSurfacemap >= 0)
							{
								half4 decalSurfaceColor = (half4)bindless_textures[decalSurfacemap].SampleGrad(sam, uvw.xy, decalDX, decalDY);
								decalSurfaceAccumulation = mad(1 - decalSurfaceAccumulationAlpha, decalColor.a * decalSurfaceColor, decalSurfaceAccumulation);
								decalSurfaceAccumulationAlpha = mad(1 - decalColor.a, decalSurfaceAccumulationAlpha, decalColor.a);
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

			baseColor.rgb = mad(baseColor.rgb, 1 - decalAccumulation.a, decalAccumulation.rgb);
			bumpColor.rgb = mad(bumpColor.rgb, 1 - decalBumpAccumulation.a, decalBumpAccumulation.rgb);
			surfaceMap = mad(surfaceMap, 1 - decalSurfaceAccumulationAlpha, decalSurfaceAccumulation);
		}
#endif // DISABLE_DECALS
#endif // SURFACE_LOAD_QUAD_DERIVATIVES

		if (any(bumpColor))
		{
			N = normalize(mul(bumpColor, TBN));
		}

		half4 specularMap = 1;
		[branch]
		if (material.textures[SPECULARMAP].IsValid() && !simple_lighting)
		{
#ifdef SURFACE_LOAD_QUAD_DERIVATIVES
			specularMap = material.textures[SPECULARMAP].SampleGrad(sam, uvsets, uvsets_dx, uvsets_dy);
#else
			float lod = 0;
#ifdef SURFACE_LOAD_MIPCONE
			lod = compute_texture_lod(material.textures[SPECULARMAP].GetTexture(), material.textures[SPECULARMAP].GetUVSet() == 0 ? lod_constant0 : lod_constant1, ray_direction, surf_normal, cone_width);
#endif // SURFACE_LOAD_MIPCONE
			specularMap = material.textures[SPECULARMAP].SampleLevel(sam, uvsets, lod);
#endif // SURFACE_LOAD_QUAD_DERIVATIVES
		}

		create(material, baseColor, surfaceMap, specularMap);

		[branch]
		if (inst.vb_wetmap >= 0)
		{
			Buffer<float> buf = bindless_buffers_float[NonUniformResourceIndex(inst.vb_wetmap)];
			const half wet0 = buf[i0];
			const half wet1 = buf[i1];
			const half wet2 = buf[i2];
			const half wet = attribute_at_bary(wet0, wet1, wet2, bary);
			if(wet > 0)
			{
				albedo = lerp(albedo, 0, wet);
				if(!is_hairparticle) // hair particle roughness modify doesn't look good because of copying surface normals
				{
					roughness = clamp(roughness * sqr(1 - wet), 0.01, 1);
					N = normalize(lerp(N, facenormal, wet));
				}
			}
		}

		transmission = material.GetTransmission();
		[branch]
		if (material.textures[TRANSMISSIONMAP].IsValid())
		{
#ifdef SURFACE_LOAD_QUAD_DERIVATIVES
			transmission *= material.textures[TRANSMISSIONMAP].SampleGrad(sam, uvsets, uvsets_dx, uvsets_dy).r;
#else
			float lod = 0;
#ifdef SURFACE_LOAD_MIPCONE
			lod = compute_texture_lod(material.textures[TRANSMISSIONMAP].GetTexture(), material.textures[TRANSMISSIONMAP].GetUVSet() == 0 ? lod_constant0 : lod_constant1, ray_direction, surf_normal, cone_width);
#endif // SURFACE_LOAD_MIPCONE
			transmission *= material.textures[TRANSMISSIONMAP].SampleLevel(sam, uvsets, lod).r;
#endif // SURFACE_LOAD_QUAD_DERIVATIVES
		}

		[branch]
		if (material.IsOcclusionEnabled_Secondary() && material.textures[OCCLUSIONMAP].IsValid())
		{
#ifdef SURFACE_LOAD_QUAD_DERIVATIVES
			occlusion *= material.textures[OCCLUSIONMAP].SampleGrad(sam, uvsets, uvsets_dx, uvsets_dy).r;
#else
			float lod = 0;
#ifdef SURFACE_LOAD_MIPCONE
			lod = compute_texture_lod(material.textures[OCCLUSIONMAP].GetTexture(), material.textures[OCCLUSIONMAP].GetUVSet() == 0 ? lod_constant0 : lod_constant1, ray_direction, surf_normal, cone_width);
#endif // SURFACE_LOAD_MIPCONE
			occlusion *= material.textures[OCCLUSIONMAP].SampleLevel(sam, uvsets, lod).r;
#endif // SURFACE_LOAD_QUAD_DERIVATIVES
		}



#ifdef SHEEN
		sheen.color = material.GetSheenColor();
		sheen.roughness = material.GetSheenRoughness();

		[branch]
		if (material.textures[SHEENCOLORMAP].IsValid())
		{
#ifdef SURFACE_LOAD_QUAD_DERIVATIVES
			sheen.color *= material.textures[SHEENCOLORMAP].SampleGrad(sam, uvsets, uvsets_dx, uvsets_dy).rgb;
#else
			float lod = 0;
#ifdef SURFACE_LOAD_MIPCONE
			lod = compute_texture_lod(material.textures[SHEENCOLORMAP].GetTexture(), material.textures[SHEENCOLORMAP].GetUVSet() == 0 ? lod_constant0 : lod_constant1, ray_direction, surf_normal, cone_width);
#endif // SURFACE_LOAD_MIPCONE
			sheen.color *= material.textures[SHEENCOLORMAP].SampleLevel(sam, uvsets, lod).rgb;
#endif // SURFACE_LOAD_QUAD_DERIVATIVES
		}
		[branch]
		if (material.textures[SHEENROUGHNESSMAP].IsValid())
		{
#ifdef SURFACE_LOAD_QUAD_DERIVATIVES
			sheen.roughness *= material.textures[SHEENROUGHNESSMAP].SampleGrad(sam, uvsets, uvsets_dx, uvsets_dy).a;
#else
			float lod = 0;
#ifdef SURFACE_LOAD_MIPCONE
			lod = compute_texture_lod(material.textures[SHEENROUGHNESSMAP].GetTexture(), material.textures[SHEENROUGHNESSMAP].GetUVSet() == 0 ? lod_constant0 : lod_constant1, ray_direction, surf_normal, cone_width);
#endif // SURFACE_LOAD_MIPCONE
			sheen.roughness *= material.textures[SHEENROUGHNESSMAP].SampleLevel(sam, uvsets, lod).a;
#endif // SURFACE_LOAD_QUAD_DERIVATIVES
		}
#endif // SHEEN


#ifdef CLEARCOAT
		clearcoat.factor = material.GetClearcoat();
		clearcoat.roughness = material.GetClearcoatRoughness();
		clearcoat.N = facenormal;

		[branch]
		if (material.textures[CLEARCOATMAP].IsValid())
		{
#ifdef SURFACE_LOAD_QUAD_DERIVATIVES
			clearcoat.factor *= material.textures[CLEARCOATMAP].SampleGrad(sam, uvsets, uvsets_dx, uvsets_dy).r;
#else
			float lod = 0;
#ifdef SURFACE_LOAD_MIPCONE
			lod = compute_texture_lod(material.textures[CLEARCOATMAP].GetTexture(), material.textures[CLEARCOATMAP].GetUVSet() == 0 ? lod_constant0 : lod_constant1, ray_direction, surf_normal, cone_width);
#endif // SURFACE_LOAD_MIPCONE
			clearcoat.factor *= material.textures[CLEARCOATMAP].SampleLevel(sam, uvsets, lod).r;
#endif // SURFACE_LOAD_QUAD_DERIVATIVES
		}
		[branch]
		if (material.textures[CLEARCOATROUGHNESSMAP].IsValid())
		{
#ifdef SURFACE_LOAD_QUAD_DERIVATIVES
			clearcoat.roughness *= material.textures[CLEARCOATROUGHNESSMAP].SampleGrad(sam, uvsets, uvsets_dx, uvsets_dy).g;
#else
			float lod = 0;
#ifdef SURFACE_LOAD_MIPCONE
			lod = compute_texture_lod(material.textures[CLEARCOATROUGHNESSMAP].GetTexture(), material.textures[CLEARCOATROUGHNESSMAP].GetUVSet() == 0 ? lod_constant0 : lod_constant1, ray_direction, surf_normal, cone_width);
#endif // SURFACE_LOAD_MIPCONE
			clearcoat.roughness *= material.textures[CLEARCOATROUGHNESSMAP].SampleLevel(sam, uvsets, lod).g;
#endif // SURFACE_LOAD_QUAD_DERIVATIVES
		}
		[branch]
		if (material.textures[CLEARCOATNORMALMAP].IsValid() && geometry.vb_tan >= 0) // also check that tan is valid! (for TBN)
		{
#ifdef SURFACE_LOAD_QUAD_DERIVATIVES
			half3 clearcoatNormalMap = half3(material.textures[CLEARCOATNORMALMAP].SampleGrad(sam, uvsets, uvsets_dx, uvsets_dy).rg, 1);
#else
			float lod = 0;
#ifdef SURFACE_LOAD_MIPCONE
			lod = compute_texture_lod(material.textures[CLEARCOATNORMALMAP].GetTexture(), material.textures[CLEARCOATNORMALMAP].GetUVSet() == 0 ? lod_constant0 : lod_constant1, ray_direction, surf_normal, cone_width);
#endif // SURFACE_LOAD_MIPCONE
			half3 clearcoatNormalMap = half3(material.textures[CLEARCOATNORMALMAP].SampleLevel(sam, uvsets, lod).rg, 1);
#endif // SURFACE_LOAD_QUAD_DERIVATIVES

			clearcoatNormalMap = clearcoatNormalMap * 2 - 1;
			clearcoat.N = mul(clearcoatNormalMap, TBN);
		}

		clearcoat.N = normalize(clearcoat.N);

#endif // CLEARCOAT

		float3 pre0;
		float3 pre1;
		float3 pre2;
		[branch]
		if (geometry.vb_pre >= 0)
		{
			Buffer<float4> buf = bindless_buffers_float4[NonUniformResourceIndex(geometry.vb_pre)];
			pre0 = buf[i0].xyz;
			pre1 = buf[i1].xyz;
			pre2 = buf[i2].xyz;
		}
		else
		{
			pre0 = data0.xyz;
			pre1 = data1.xyz;
			pre2 = data2.xyz;
		}

#ifdef SURFACE_LOAD_ENABLE_WIND
		// Need interpolated wind transform:
		pre0 = mul(inst.transformPrev.GetMatrix(), float4(pre0, 1)).xyz;
		pre1 = mul(inst.transformPrev.GetMatrix(), float4(pre1, 1)).xyz;
		pre2 = mul(inst.transformPrev.GetMatrix(), float4(pre2, 1)).xyz;
		
		[branch]
		if (material.IsUsingWind())
		{
			pre0 += sample_wind_prev(pre0, data0.w);
			pre1 += sample_wind_prev(pre1, data1.w);
			pre2 += sample_wind_prev(pre2, data2.w);
		}
		pre = attribute_at_bary(pre0, pre1, pre2, bary);
#else
		// Simplified matrix transform is allowed without wind:
		pre = attribute_at_bary(pre0, pre1, pre2, bary);
		pre = mul(inst.transformPrev.GetMatrix(), float4(pre, 1)).xyz;
#endif // SURFACE_LOAD_ENABLE_WIND

		update();
	}

	bool load(in PrimitiveID prim, in float2 barycentrics)
	{
		if (!preload_internal(prim))
			return false;

		bary = barycentrics;

		float3 p0 = data0.xyz;
		float3 p1 = data1.xyz;
		float3 p2 = data2.xyz;

#ifdef SURFACE_LOAD_QUAD_DERIVATIVES
		float3 P0 = mul(inst.transform.GetMatrix(), float4(p0, 1)).xyz;
		float3 P1 = mul(inst.transform.GetMatrix(), float4(p1, 1)).xyz;
		float3 P2 = mul(inst.transform.GetMatrix(), float4(p2, 1)).xyz;
		P = attribute_at_bary(P0, P1, P2, bary);
		P_dx = P - attribute_at_bary(P0, P1, P2, bary_quad_x);
		P_dy = P - attribute_at_bary(P0, P1, P2, bary_quad_y);
#else
		P = attribute_at_bary(p0, p1, p2, bary);
		P = mul(inst.transform.GetMatrix(), float4(P, 1)).xyz;
#endif // SURFACE_LOAD_QUAD_DERIVATIVES

		load_internal();
		return true;
	}
	bool load(in PrimitiveID prim, in float3 worldPosition)
	{
		if (!preload_internal(prim))
			return false;

		float3 p0 = data0.xyz;
		float3 p1 = data1.xyz;
		float3 p2 = data2.xyz;
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

		float3 p0 = data0.xyz;
		float3 p1 = data1.xyz;
		float3 p2 = data2.xyz;
		float3 P0 = mul(inst.transform.GetMatrix(), float4(p0, 1)).xyz;
		float3 P1 = mul(inst.transform.GetMatrix(), float4(p1, 1)).xyz;
		float3 P2 = mul(inst.transform.GetMatrix(), float4(p2, 1)).xyz;

#ifdef SURFACE_LOAD_ENABLE_WIND
		[branch]
		if (material.IsUsingWind())
		{
			P0 += sample_wind(P0, data0.w);
			P1 += sample_wind(P1, data1.w);
			P2 += sample_wind(P2, data2.w);
		}
#endif // SURFACE_LOAD_ENABLE_WIND

		bary = compute_barycentrics(rayOrigin, rayDirection, P0, P1, P2, hit_depth, is_backface);
		P = rayOrigin + rayDirection * hit_depth;
		V = -rayDirection;

#ifdef SURFACE_LOAD_QUAD_DERIVATIVES
		bary_quad_x = compute_barycentrics(rayOrigin, QuadReadAcrossX(rayDirection), P0, P1, P2);
		bary_quad_y = compute_barycentrics(rayOrigin, QuadReadAcrossY(rayDirection), P0, P1, P2);
		P_dx = P - attribute_at_bary(P0, P1, P2, bary_quad_x);
		P_dy = P - attribute_at_bary(P0, P1, P2, bary_quad_y);
#endif // SURFACE_LOAD_QUAD_DERIVATIVES

		load_internal();
		return true;
	}
	bool load(in PrimitiveID prim, in float3 rayOrigin, in float3 rayDirection, in float3 rayDirection_quad_x, in float3 rayDirection_quad_y, in uint flatTileIndex = 0)
	{
		if (!preload_internal(prim))
			return false;

		float3 p0 = data0.xyz;
		float3 p1 = data1.xyz;
		float3 p2 = data2.xyz;
		float3 P0 = mul(inst.transform.GetMatrix(), float4(p0, 1)).xyz;
		float3 P1 = mul(inst.transform.GetMatrix(), float4(p1, 1)).xyz;
		float3 P2 = mul(inst.transform.GetMatrix(), float4(p2, 1)).xyz;

#ifdef SURFACE_LOAD_ENABLE_WIND
		[branch]
		if (material.IsUsingWind())
		{
			P0 += sample_wind(P0, data0.w);
			P1 += sample_wind(P1, data1.w);
			P2 += sample_wind(P2, data2.w);
		}
#endif // SURFACE_LOAD_ENABLE_WIND

		bary = compute_barycentrics(rayOrigin, rayDirection, P0, P1, P2, hit_depth, is_backface);
		P = rayOrigin + rayDirection * hit_depth;
		V = -rayDirection;

#ifdef SURFACE_LOAD_QUAD_DERIVATIVES
		bary_quad_x = compute_barycentrics(rayOrigin, rayDirection_quad_x, P0, P1, P2);
		bary_quad_y = compute_barycentrics(rayOrigin, rayDirection_quad_y, P0, P1, P2);
		P_dx = P - attribute_at_bary(P0, P1, P2, bary_quad_x);
		P_dy = P - attribute_at_bary(P0, P1, P2, bary_quad_y);
#endif // SURFACE_LOAD_QUAD_DERIVATIVES

		load_internal(flatTileIndex);
		return true;
	}
};

#endif // WI_SURFACE_HF
