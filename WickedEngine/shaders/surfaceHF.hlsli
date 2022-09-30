#ifndef WI_SURFACE_HF
#define WI_SURFACE_HF
#include "globals.hlsli"

#define max3(v) max(max(v.x, v.y), v.z)

float3 F_Schlick(const float3 f0, float f90, float VoH)
{
	// Schlick 1994, "An Inexpensive BRDF Model for Physically-Based Rendering"
	return f0 + (f90 - f0) * pow5(1.0 - VoH);
}

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

	float4 baseColor;
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
	float4 T;				// tangent
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
	float3 gi;
	float3 bumpColor;

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
		anisotropy = 0;
		sss = 0;
		sss_inv = 1;
		layerMask = ~0;
		facenormal = 0;
		flags = 0;
		gi = 0;
		bumpColor = 0;

		sheen.color = 0;
		sheen.roughness = 0;

		clearcoat.factor = 0;
		clearcoat.roughness = 0;
		clearcoat.N = 0;

		uid_validate = 0;
		raycone = (RayCone)0;
		hit_depth = 0;
	}

	inline void create(in ShaderMaterial material)
	{
		sss = material.subsurfaceScattering;
		sss_inv = material.subsurfaceScattering_inv;

		layerMask = material.layerMask;

		if (material.IsReceiveShadow())
		{
			flags |= SURFACE_FLAG_RECEIVE_SHADOW;
		}

#ifdef ANISOTROPIC
		anisotropy = material.parallaxOcclusionMapping;
#endif // ANISOTROPIC
	}

	inline void create(
		in ShaderMaterial material,
		in float4 _baseColor,
		in float4 surfaceMap,
		in float4 specularMap = 1
	)
	{
		baseColor = _baseColor;
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

#ifndef ENVMAPRENDERING
		if (GetFrame().options & OPTION_BIT_FORCE_DIFFUSE_LIGHTING)
#endif // ENVMAPRENDERING
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

#ifdef UNLIT
		albedo = baseColor.rgb;
#endif // UNLIT

		create(material);
	}

	inline void update()
	{
		roughness = clamp(roughness, 0.045, 1);
		roughnessBRDF = roughness * roughness;

		sheen.roughness = clamp(sheen.roughness, 0.045, 1);
		sheen.roughnessBRDF = sheen.roughness * sheen.roughness;

		clearcoat.roughness = clamp(clearcoat.roughness, 0.045, 1);
		clearcoat.roughnessBRDF = clearcoat.roughness * clearcoat.roughness;

		NdotV = saturate(dot(N, V) + 1e-5);

		f90 = saturate(50.0 * dot(f0, 0.33));
		F = F_Schlick(f0, f90, NdotV);
		clearcoat.F = F_Schlick(f0, f90, saturate(dot(clearcoat.N, V) + 1e-5));
		clearcoat.F *= clearcoat.factor;

		R = -reflect(V, N);
		clearcoat.R = -reflect(V, clearcoat.N);

		// Sheen energy compensation: https://dassaultsystemes-technology.github.io/EnterprisePBRShadingModel/spec-2021x.md.html#figure_energy-compensation-sheen-e
		sheen.DFG = texture_sheenlut.SampleLevel(sampler_linear_clamp, float2(NdotV, sheen.roughness), 0).r;
		sheen.albedoScaling = 1.0 - max3(sheen.color) * sheen.DFG;

		B = normalize(cross(T.xyz, N) * T.w); // Compute bitangent again after normal mapping
		TdotV = dot(T.xyz, V);
		BdotV = dot(B, V);
		at = max(0, roughnessBRDF * (1 + anisotropy));
		ab = max(0, roughnessBRDF * (1 - anisotropy));

#ifdef CARTOON
		F = smoothstep(0.1, 0.5, F);
#endif // CARTOON
	}

	inline bool IsReceiveShadow() { return flags & SURFACE_FLAG_RECEIVE_SHADOW; }


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

		material = load_material(geometry.materialIndex);
		create(material);

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
		SamplerState sam = bindless_samplers[GetFrame().sampler_objectshader_index];

		const bool is_hairparticle = geometry.flags & SHADERMESH_FLAG_HAIRPARTICLE;
		const bool is_emittedparticle = geometry.flags & SHADERMESH_FLAG_EMITTEDPARTICLE;
		const bool simple_lighting = is_hairparticle || is_emittedparticle;

		float3 n0 = unpack_unitvector(data0.w);
		float3 n1 = unpack_unitvector(data1.w);
		float3 n2 = unpack_unitvector(data2.w);
		N = attribute_at_bary(n0, n1, n2, bary);
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
		float4 uvsets_dx = 0;
		float4 uvsets_dy = 0;
		[branch]
		if (geometry.vb_uvs >= 0)
		{
			ByteAddressBuffer buf = bindless_buffers[NonUniformResourceIndex(geometry.vb_uvs)];
			float4 uv0 = unpack_half4(buf.Load2(i0 * sizeof(uint2)));
			float4 uv1 = unpack_half4(buf.Load2(i1 * sizeof(uint2)));
			float4 uv2 = unpack_half4(buf.Load2(i2 * sizeof(uint2)));
			// all three must be transformed, to have correct derivatives (not enough to only transform final uvsets):
			uv0.xy = mad(uv0.xy, material.texMulAdd.xy, material.texMulAdd.zw);
			uv1.xy = mad(uv1.xy, material.texMulAdd.xy, material.texMulAdd.zw);
			uv2.xy = mad(uv2.xy, material.texMulAdd.xy, material.texMulAdd.zw);
			uvsets = attribute_at_bary(uv0, uv1, uv2, bary);

#ifdef SURFACE_LOAD_MIPCONE
			lod_constant0 = 0.5 * log2(twice_uv_area(uv0.xy, uv1.xy, uv2.xy) * triangle_constant);
			lod_constant1 = 0.5 * log2(twice_uv_area(uv0.zw, uv1.zw, uv2.zw) * triangle_constant);
#endif // SURFACE_LOAD_MIPCONE

#ifdef SURFACE_LOAD_QUAD_DERIVATIVES
			uvsets_dx = uvsets - attribute_at_bary(uv0, uv1, uv2, bary_quad_x);
			uvsets_dy = uvsets - attribute_at_bary(uv0, uv1, uv2, bary_quad_y);
#endif // SURFACE_LOAD_QUAD_DERIVATIVES
		}

		[branch]
		if (geometry.vb_tan >= 0)
		{
			ByteAddressBuffer buf = bindless_buffers[NonUniformResourceIndex(geometry.vb_tan)];
			const float4 t0 = unpack_utangent(buf.Load(i0 * sizeof(uint)));
			const float4 t1 = unpack_utangent(buf.Load(i1 * sizeof(uint)));
			const float4 t2 = unpack_utangent(buf.Load(i2 * sizeof(uint)));
			T = attribute_at_bary(t0, t1, t2, bary);
			T = T * 2 - 1;
			T.xyz = mul((float3x3)inst.transformInverseTranspose.GetMatrix(), T.xyz);
			T.xyz = normalize(T.xyz);
			B = normalize(cross(T.xyz, N) * T.w);
			const float3x3 TBN = float3x3(T.xyz, B, N);

#ifdef PARALLAXOCCLUSIONMAPPING
			[branch]
			if (material.texture_displacementmap_index >= 0)
			{
				const float2 uv = material.uvset_displacementMap == 0 ? uvsets.xy : uvsets.zw;
				const float2 uv_dx = material.uvset_displacementMap == 0 ? uvsets_dx.xy : uvsets_dx.zw;
				const float2 uv_dy = material.uvset_displacementMap == 0 ? uvsets_dy.xy : uvsets_dy.zw;
				Texture2D tex = bindless_textures[NonUniformResourceIndex(material.texture_displacementmap_index)];
				ParallaxOcclusionMapping_Impl(
					uvsets,
					V,
					TBN,
					material,
					tex,
					uv,
					uv_dx,
					uv_dy
				);
			}
#endif // PARALLAXOCCLUSIONMAPPING

			// Normal mapping:
			[branch]
			if (geometry.vb_tan >= 0 && material.texture_normalmap_index >= 0 && material.normalMapStrength > 0)
			{
				const float2 uv = material.uvset_normalMap == 0 ? uvsets.xy : uvsets.zw;
				Texture2D tex = bindless_textures[NonUniformResourceIndex(material.texture_normalmap_index)];
#ifdef SURFACE_LOAD_QUAD_DERIVATIVES
				const float2 uv_dx = material.uvset_normalMap == 0 ? uvsets_dx.xy : uvsets_dx.zw;
				const float2 uv_dy = material.uvset_normalMap == 0 ? uvsets_dy.xy : uvsets_dy.zw;
				const float3 normalMap = float3(tex.SampleGrad(sam, uv, uv_dx, uv_dy).rg, 1) * 2 - 1;
#else
				float lod = 0;
#ifdef SURFACE_LOAD_MIPCONE
				lod = compute_texture_lod(tex, material.uvset_normalMap == 0 ? lod_constant0 : lod_constant1, ray_direction, surf_normal, cone_width);
#endif // SURFACE_LOAD_MIPCONE
				const float3 normalMap = float3(tex.SampleLevel(sam, uv, lod).rg, 1) * 2 - 1;
#endif // SURFACE_LOAD_QUAD_DERIVATIVES
				N = normalize(lerp(N, mul(normalMap, TBN), material.normalMapStrength));
				bumpColor = normalMap * material.normalMapStrength;
			}
		}

		baseColor = is_emittedparticle ? 1 : material.baseColor;
		baseColor *= unpack_rgba(inst.color);
		[branch]
		if (material.texture_basecolormap_index >= 0)
		{
			const float2 uv = material.uvset_baseColorMap == 0 ? uvsets.xy : uvsets.zw;
			Texture2D tex = bindless_textures[NonUniformResourceIndex(material.texture_basecolormap_index)];
#ifdef SURFACE_LOAD_QUAD_DERIVATIVES
			const float2 uv_dx = material.uvset_baseColorMap == 0 ? uvsets_dx.xy : uvsets_dx.zw;
			const float2 uv_dy = material.uvset_baseColorMap == 0 ? uvsets_dy.xy : uvsets_dy.zw;
			float4 baseColorMap = tex.SampleGrad(sam, uv, uv_dx, uv_dy);
#else
			float lod = 0;
#ifdef SURFACE_LOAD_MIPCONE
			lod = compute_texture_lod(tex, material.uvset_baseColorMap == 0 ? lod_constant0 : lod_constant1, ray_direction, surf_normal, cone_width);
#endif // SURFACE_LOAD_MIPCONE
			float4 baseColorMap = tex.SampleLevel(sam, uv, lod);
#endif // SURFACE_LOAD_QUAD_DERIVATIVES
			if ((GetFrame().options & OPTION_BIT_DISABLE_ALBEDO_MAPS) == 0)
			{
				baseColorMap.rgb = DEGAMMA(baseColorMap.rgb);
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
			float4 vertexColor = attribute_at_bary(c0, c1, c2, bary);
			baseColor *= vertexColor;
		}

		[branch]
		if (inst.lightmap >= 0 && geometry.vb_atl >= 0)
		{
			ByteAddressBuffer buf = bindless_buffers[NonUniformResourceIndex(geometry.vb_atl)];
			const float2 a0 = unpack_half2(buf.Load(i0 * sizeof(uint)));
			const float2 a1 = unpack_half2(buf.Load(i1 * sizeof(uint)));
			const float2 a2 = unpack_half2(buf.Load(i2 * sizeof(uint)));
			float2 atlas = attribute_at_bary(a0, a1, a2, bary);

			Texture2D<float4> tex = bindless_textures[NonUniformResourceIndex(inst.lightmap)];
			gi = tex.SampleLevel(sampler_linear_clamp, atlas, 0).rgb;

			flags |= SURFACE_FLAG_GI_APPLIED;
		}

		float4 surfaceMap = 1;
		[branch]
		if (material.texture_surfacemap_index >= 0 && !simple_lighting)
		{
			const float2 uv = material.uvset_surfaceMap == 0 ? uvsets.xy : uvsets.zw;
			Texture2D tex = bindless_textures[NonUniformResourceIndex(material.texture_surfacemap_index)];
#ifdef SURFACE_LOAD_QUAD_DERIVATIVES
			const float2 uv_dx = material.uvset_surfaceMap == 0 ? uvsets_dx.xy : uvsets_dx.zw;
			const float2 uv_dy = material.uvset_surfaceMap == 0 ? uvsets_dy.xy : uvsets_dy.zw;
			surfaceMap = tex.SampleGrad(sam, uv, uv_dx, uv_dy);
#else
			float lod = 0;
#ifdef SURFACE_LOAD_MIPCONE
			lod = compute_texture_lod(tex, material.uvset_surfaceMap == 0 ? lod_constant0 : lod_constant1, ray_direction, surf_normal, cone_width);
#endif // SURFACE_LOAD_MIPCONE
			surfaceMap = tex.SampleLevel(sam, uv, lod);
#endif // SURFACE_LOAD_QUAD_DERIVATIVES
		}
		if (simple_lighting)
		{
			surfaceMap = 0;
		}

		float4 specularMap = 1;
		[branch]
		if (material.texture_specularmap_index >= 0 && !simple_lighting)
		{
			const float2 uv = material.uvset_specularMap == 0 ? uvsets.xy : uvsets.zw;
			Texture2D tex = bindless_textures[NonUniformResourceIndex(material.texture_specularmap_index)];
#ifdef SURFACE_LOAD_QUAD_DERIVATIVES
			const float2 uv_dx = material.uvset_specularMap == 0 ? uvsets_dx.xy : uvsets_dx.zw;
			const float2 uv_dy = material.uvset_specularMap == 0 ? uvsets_dy.xy : uvsets_dy.zw;
			specularMap = tex.SampleGrad(sam, uv, uv_dx, uv_dy);
#else
			float lod = 0;
#ifdef SURFACE_LOAD_MIPCONE
			lod = compute_texture_lod(tex, material.uvset_specularMap == 0 ? lod_constant0 : lod_constant1, ray_direction, surf_normal, cone_width);
#endif // SURFACE_LOAD_MIPCONE
			specularMap = tex.SampleLevel(sam, uv, lod);
#endif // SURFACE_LOAD_QUAD_DERIVATIVES
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
				const float2 uv = material.uvset_emissiveMap == 0 ? uvsets.xy : uvsets.zw;
				Texture2D tex = bindless_textures[NonUniformResourceIndex(material.texture_emissivemap_index)];
#ifdef SURFACE_LOAD_QUAD_DERIVATIVES
				const float2 uv_dx = material.uvset_emissiveMap == 0 ? uvsets_dx.xy : uvsets_dx.zw;
				const float2 uv_dy = material.uvset_emissiveMap == 0 ? uvsets_dy.xy : uvsets_dy.zw;
				float4 emissiveMap = tex.SampleGrad(sam, uv, uv_dx, uv_dy);
#else
				float lod = 0;
#ifdef SURFACE_LOAD_MIPCONE
				lod = compute_texture_lod(tex, material.uvset_emissiveMap == 0 ? lod_constant0 : lod_constant1, ray_direction, surf_normal, cone_width);
#endif // SURFACE_LOAD_MIPCONE
				float4 emissiveMap = tex.SampleLevel(sam, uv, lod);
#endif // SURFACE_LOAD_QUAD_DERIVATIVES
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
			const float2 uv = material.uvset_transmissionMap == 0 ? uvsets.xy : uvsets.zw;
			Texture2D tex = bindless_textures[NonUniformResourceIndex(material.texture_transmissionmap_index)];
#ifdef SURFACE_LOAD_QUAD_DERIVATIVES
			const float2 uv_dx = material.uvset_transmissionMap == 0 ? uvsets_dx.xy : uvsets_dx.zw;
			const float2 uv_dy = material.uvset_transmissionMap == 0 ? uvsets_dy.xy : uvsets_dy.zw;
			transmission *= tex.SampleGrad(sam, uv, uv_dx, uv_dy).r;
#else
			float lod = 0;
#ifdef SURFACE_LOAD_MIPCONE
			lod = compute_texture_lod(tex, material.uvset_transmissionMap == 0 ? lod_constant0 : lod_constant1, ray_direction, surf_normal, cone_width);
#endif // SURFACE_LOAD_MIPCONE
			transmission *= tex.SampleLevel(sam, uv, lod).r;
#endif // SURFACE_LOAD_QUAD_DERIVATIVES
		}

		[branch]
		if (material.IsOcclusionEnabled_Secondary() && material.texture_occlusionmap_index >= 0)
		{
			const float2 uv = material.uvset_occlusionMap == 0 ? uvsets.xy : uvsets.zw;
			Texture2D tex = bindless_textures[NonUniformResourceIndex(material.texture_occlusionmap_index)];
#ifdef SURFACE_LOAD_QUAD_DERIVATIVES
			const float2 uv_dx = material.uvset_occlusionMap == 0 ? uvsets_dx.xy : uvsets_dx.zw;
			const float2 uv_dy = material.uvset_occlusionMap == 0 ? uvsets_dy.xy : uvsets_dy.zw;
			occlusion *= tex.SampleGrad(sam, uv, uv_dx, uv_dy).r;
#else
			float lod = 0;
#ifdef SURFACE_LOAD_MIPCONE
			lod = compute_texture_lod(tex, material.uvset_occlusionMap == 0 ? lod_constant0 : lod_constant1, ray_direction, surf_normal, cone_width);
#endif // SURFACE_LOAD_MIPCONE
			occlusion *= tex.SampleLevel(sam, uv, lod).r;
#endif // SURFACE_LOAD_QUAD_DERIVATIVES
		}

#ifdef SHEEN
		sheen.color = material.GetSheenColor();
		sheen.roughness = material.sheenRoughness;

		[branch]
		if (material.texture_sheencolormap_index >= 0)
		{
			const float2 uv = material.uvset_sheenColorMap == 0 ? uvsets.xy : uvsets.zw;
			Texture2D tex = bindless_textures[NonUniformResourceIndex(material.texture_sheencolormap_index)];
#ifdef SURFACE_LOAD_QUAD_DERIVATIVES
			const float2 uv_dx = material.uvset_sheenColorMap == 0 ? uvsets_dx.xy : uvsets_dx.zw;
			const float2 uv_dy = material.uvset_sheenColorMap == 0 ? uvsets_dy.xy : uvsets_dy.zw;
			sheen.color *= DEGAMMA(tex.SampleGrad(sam, uv, uv_dx, uv_dy).rgb);
#else
			float lod = 0;
#ifdef SURFACE_LOAD_MIPCONE
			lod = compute_texture_lod(tex, material.uvset_sheenColorMap == 0 ? lod_constant0 : lod_constant1, ray_direction, surf_normal, cone_width);
#endif // SURFACE_LOAD_MIPCONE
			sheen.color *= DEGAMMA(tex.SampleLevel(sam, uv, lod).rgb);
#endif // SURFACE_LOAD_QUAD_DERIVATIVES
		}
		[branch]
		if (material.texture_sheenroughnessmap_index >= 0)
		{
			const float2 uv = material.uvset_sheenRoughnessMap == 0 ? uvsets.xy : uvsets.zw;
			Texture2D tex = bindless_textures[NonUniformResourceIndex(material.texture_sheenroughnessmap_index)];
#ifdef SURFACE_LOAD_QUAD_DERIVATIVES
			const float2 uv_dx = material.uvset_sheenRoughnessMap == 0 ? uvsets_dx.xy : uvsets_dx.zw;
			const float2 uv_dy = material.uvset_sheenRoughnessMap == 0 ? uvsets_dy.xy : uvsets_dy.zw;
			sheen.roughness *= tex.SampleGrad(sam, uv, uv_dx, uv_dy).a;
#else
			float lod = 0;
#ifdef SURFACE_LOAD_MIPCONE
			lod = compute_texture_lod(tex, material.uvset_sheenRoughnessMap == 0 ? lod_constant0 : lod_constant1, ray_direction, surf_normal, cone_width);
#endif // SURFACE_LOAD_MIPCONE
			sheen.roughness *= tex.SampleLevel(sam, uv, lod).a;
#endif // SURFACE_LOAD_QUAD_DERIVATIVES
		}
#endif // SHEEN


#ifdef CLEARCOAT
		clearcoat.factor = material.clearcoat;
		clearcoat.roughness = material.clearcoatRoughness;
		clearcoat.N = facenormal;

		[branch]
		if (material.texture_clearcoatmap_index >= 0)
		{
			const float2 uv = material.uvset_clearcoatMap == 0 ? uvsets.xy : uvsets.zw;
			Texture2D tex = bindless_textures[NonUniformResourceIndex(material.texture_clearcoatmap_index)];
#ifdef SURFACE_LOAD_QUAD_DERIVATIVES
			const float2 uv_dx = material.uvset_clearcoatMap == 0 ? uvsets_dx.xy : uvsets_dx.zw;
			const float2 uv_dy = material.uvset_clearcoatMap == 0 ? uvsets_dy.xy : uvsets_dy.zw;
			clearcoat.factor *= tex.SampleGrad(sam, uv, uv_dx, uv_dy).r;
#else
			float lod = 0;
#ifdef SURFACE_LOAD_MIPCONE
			lod = compute_texture_lod(tex, material.uvset_clearcoatMap == 0 ? lod_constant0 : lod_constant1, ray_direction, surf_normal, cone_width);
#endif // SURFACE_LOAD_MIPCONE
			clearcoat.factor *= tex.SampleLevel(sam, uv, lod).r;
#endif // SURFACE_LOAD_QUAD_DERIVATIVES
		}
		[branch]
		if (material.texture_clearcoatroughnessmap_index >= 0)
		{
			const float2 uv = material.uvset_clearcoatRoughnessMap == 0 ? uvsets.xy : uvsets.zw;
			Texture2D tex = bindless_textures[NonUniformResourceIndex(material.texture_clearcoatroughnessmap_index)];
#ifdef SURFACE_LOAD_QUAD_DERIVATIVES
			const float2 uv_dx = material.uvset_clearcoatRoughnessMap == 0 ? uvsets_dx.xy : uvsets_dx.zw;
			const float2 uv_dy = material.uvset_clearcoatRoughnessMap == 0 ? uvsets_dy.xy : uvsets_dy.zw;
			clearcoat.roughness *= tex.SampleGrad(sam, uv, uv_dx, uv_dy).g;
#else
			float lod = 0;
#ifdef SURFACE_LOAD_MIPCONE
			lod = compute_texture_lod(tex, material.uvset_clearcoatRoughnessMap == 0 ? lod_constant0 : lod_constant1, ray_direction, surf_normal, cone_width);
#endif // SURFACE_LOAD_MIPCONE
			clearcoat.roughness *= tex.SampleLevel(sam, uv, lod).g;
#endif // SURFACE_LOAD_QUAD_DERIVATIVES
		}
		[branch]
		if (material.texture_clearcoatnormalmap_index >= 0 && geometry.vb_tan >= 0) // also check that tan is valid! (for TBN)
		{
			const float2 uv = material.uvset_clearcoatNormalMap == 0 ? uvsets.xy : uvsets.zw;
			Texture2D tex = bindless_textures[NonUniformResourceIndex(material.texture_clearcoatnormalmap_index)];
#ifdef SURFACE_LOAD_QUAD_DERIVATIVES
			const float2 uv_dx = material.uvset_clearcoatNormalMap == 0 ? uvsets_dx.xy : uvsets_dx.zw;
			const float2 uv_dy = material.uvset_clearcoatNormalMap == 0 ? uvsets_dy.xy : uvsets_dy.zw;
			float3 clearcoatNormalMap = float3(tex.SampleGrad(sam, uv, uv_dx, uv_dy).rg, 1);
#else
			float lod = 0;
#ifdef SURFACE_LOAD_MIPCONE
			lod = compute_texture_lod(tex, material.uvset_clearcoatNormalMap == 0 ? lod_constant0 : lod_constant1, ray_direction, surf_normal, cone_width);
#endif // SURFACE_LOAD_MIPCONE
			float3 clearcoatNormalMap = float3(tex.SampleLevel(sam, uv, lod).rg, 1);
#endif // SURFACE_LOAD_QUAD_DERIVATIVES

			clearcoatNormalMap = clearcoatNormalMap * 2 - 1;
			const float3x3 TBN = float3x3(T.xyz, B, N);
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
		pre = attribute_at_bary(pre0, pre1, pre2, bary);
		pre = mul(inst.transformPrev.GetMatrix(), float4(pre, 1)).xyz;

		update();
	}

	bool load(in PrimitiveID prim, in float2 barycentrics)
	{
		if (!preload_internal(prim))
			return false;

		bary = barycentrics;

		float3 p0 = asfloat(data0.xyz);
		float3 p1 = asfloat(data1.xyz);
		float3 p2 = asfloat(data2.xyz);

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

#ifdef SURFACE_LOAD_ENABLE_WIND
		[branch]
		if (material.IsUsingWind())
		{
			float wind0 = ((data0.w >> 24u) & 0xFF) / 255.0;
			float wind1 = ((data1.w >> 24u) & 0xFF) / 255.0;
			float wind2 = ((data2.w >> 24u) & 0xFF) / 255.0;

			// this is hella slow to do per pixel:
			P0 += compute_wind(P0, wind0);
			P1 += compute_wind(P1, wind1);
			P2 += compute_wind(P2, wind2);
		}
#endif // SURFACE_LOAD_ENABLE_WIND

		bool is_backface;
		bary = compute_barycentrics(rayOrigin, rayDirection, P0, P1, P2, hit_depth, is_backface);
		P = rayOrigin + rayDirection * hit_depth;
		V = -rayDirection;
		if (is_backface)
		{
			flags |= SURFACE_FLAG_BACKFACE;
		}

#ifdef SURFACE_LOAD_QUAD_DERIVATIVES
		bary_quad_x = compute_barycentrics(rayOrigin, QuadReadAcrossX(rayDirection), P0, P1, P2);
		bary_quad_y = compute_barycentrics(rayOrigin, QuadReadAcrossY(rayDirection), P0, P1, P2);
		P_dx = P - attribute_at_bary(P0, P1, P2, bary_quad_x);
		P_dy = P - attribute_at_bary(P0, P1, P2, bary_quad_y);
#endif // SURFACE_LOAD_QUAD_DERIVATIVES

		load_internal();
		return true;
	}
	bool load(in PrimitiveID prim, in float3 rayOrigin, in float3 rayDirection, in float3 rayDirection_quad_x, in float3 rayDirection_quad_y)
	{
		if (!preload_internal(prim))
			return false;

		float3 p0 = asfloat(data0.xyz);
		float3 p1 = asfloat(data1.xyz);
		float3 p2 = asfloat(data2.xyz);
		float3 P0 = mul(inst.transform.GetMatrix(), float4(p0, 1)).xyz;
		float3 P1 = mul(inst.transform.GetMatrix(), float4(p1, 1)).xyz;
		float3 P2 = mul(inst.transform.GetMatrix(), float4(p2, 1)).xyz;

#ifdef SURFACE_LOAD_ENABLE_WIND
		[branch]
		if (material.IsUsingWind())
		{
			float wind0 = ((data0.w >> 24u) & 0xFF) / 255.0;
			float wind1 = ((data1.w >> 24u) & 0xFF) / 255.0;
			float wind2 = ((data2.w >> 24u) & 0xFF) / 255.0;

			// this is hella slow to do per pixel:
			P0 += compute_wind(P0, wind0);
			P1 += compute_wind(P1, wind1);
			P2 += compute_wind(P2, wind2);
		}
#endif // SURFACE_LOAD_ENABLE_WIND

		bool is_backface;
		bary = compute_barycentrics(rayOrigin, rayDirection, P0, P1, P2, hit_depth, is_backface);
		P = rayOrigin + rayDirection * hit_depth;
		V = -rayDirection;
		if (is_backface)
		{
			flags |= SURFACE_FLAG_BACKFACE;
		}

#ifdef SURFACE_LOAD_QUAD_DERIVATIVES
		bary_quad_x = compute_barycentrics(rayOrigin, rayDirection_quad_x, P0, P1, P2);
		bary_quad_y = compute_barycentrics(rayOrigin, rayDirection_quad_y, P0, P1, P2);
		P_dx = P - attribute_at_bary(P0, P1, P2, bary_quad_x);
		P_dy = P - attribute_at_bary(P0, P1, P2, bary_quad_y);
#endif // SURFACE_LOAD_QUAD_DERIVATIVES

		load_internal();
		return true;
	}
};

#endif // WI_SURFACE_HF
