#ifndef WI_BRDF_HF
#define WI_BRDF_HF
#include "globals.hlsli"

float3 F_Schlick(in float3 f0, in float f90, in float u)
{
	return f0 + (f90 - f0) * pow(1 - u, 5);
}
float3 F_Fresnel(float3 SpecularColor, float VoH)
{
	float3 SpecularColorSqrt = sqrt(min(SpecularColor, float3(0.99, 0.99, 0.99)));
	float3 n = (1 + SpecularColorSqrt) / (1 - SpecularColorSqrt);
	float3 g = sqrt(n * n + VoH * VoH - 1);
	return 0.5 * sqr((g - VoH) / (g + VoH)) * (1 + sqr(((g + VoH) * VoH - 1) / ((g - VoH) * VoH + 1)));
}



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
	float4 emissiveColor;	// light emission [0 -> 1]
	float4 refraction;		// refraction color (rgb), refraction amount (a)
	float2 pixel;			// pixel coordinate (used for randomization effects)
	float2 screenUV;		// pixel coordinate in UV space [0 -> 1] (used for randomization effects)
	float3 T;				// tangent
	float3 B;				// bitangent
	float anisotropy;		// anisotropy factor [0 -> 1]
	float4 sss;				// subsurface scattering color * amount
	float4 sss_inv;			// 1 / (1 + sss)
	uint layerMask;

	// These will be computed when calling Update():
	float alphaRoughness;	// roughness remapped from perceptual to a "more linear change in roughness"
	float alphaRoughnessSq;	// roughness input to brdf functions
	float NdotV;			// cos(angle between normal and view vector)
	float f90;				// reflectance at grazing angle
	float3 R;				// reflection vector
	float3 F;				// fresnel term computed from NdotV
	float TdotV;
	float BdotV;
	float at;
	float ab;

	inline void init()
	{
		albedo = 1;
		f0 = 0;
		roughness = 1;
		occlusion = 1;
		emissiveColor = 0;
		refraction = 0;
		pixel = 0;
		screenUV = 0;
		T = 0;
		B = 0;
		anisotropy = 0;
		sss = 0;
		sss_inv = 1;
		layerMask = ~0;
	}

	inline void create(
		in ShaderMaterial material,
		in float4 baseColor,
		in float4 surfaceMap
	)
	{
		init();

		roughness = material.roughness;
		f0 = material.specularColor.rgb * material.specularColor.a;

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
	}

	inline void update()
	{
		alphaRoughness = roughness * roughness;
		alphaRoughnessSq = alphaRoughness * alphaRoughness;

		NdotV = abs(dot(N, V)) + 1e-5;

		f90 = saturate(50.0 * dot(f0, 0.33));
		R = -reflect(V, N);
		F = F_Schlick(f0, f90, NdotV);

		TdotV = dot(T, V);
		BdotV = dot(B, V);
		at = max(0, alphaRoughness * (1 + anisotropy));
		ab = max(0, alphaRoughness * (1 - anisotropy));

#ifdef BRDF_CARTOON
		F = smoothstep(0.05, 0.1, F);
#endif // BRDF_CARTOON
	}
};

struct SurfaceToLight
{
	float3 L;		// surface to light vector (normalized)
	float3 H;		// half-vector between view vector and light vector
	float NdotL;	// cos angle between normal and light direction
	float3 NdotL_sss;	// NdotL with subsurface parameters applied
	float NdotV;	// cos angle between normal and view direction
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

		NdotL_sss = (NdotL + surface.sss.rgb) * surface.sss_inv.rgb;

		NdotV = saturate(dot(surface.N, surface.V));
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

// Smith Joint GGX
// Note: Vis = G / (4 * NdotL * NdotV)
// see Eric Heitz. 2014. Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs. Journal of Computer Graphics Techniques, 3
// see Real-Time Rendering. Page 331 to 336.
// see https://google.github.io/filament/Filament.md.html#materialsystem/specularbrdf/geometricshadowing(specularg)
float visibilityOcclusion(in Surface surface, in SurfaceToLight surfaceToLight)
{
	float GGXV = surfaceToLight.NdotL * sqrt(surfaceToLight.NdotV * surfaceToLight.NdotV * (1.0 - surface.alphaRoughnessSq) + surface.alphaRoughnessSq);
	float GGXL = surfaceToLight.NdotV * sqrt(surfaceToLight.NdotL * surfaceToLight.NdotL * (1.0 - surface.alphaRoughnessSq) + surface.alphaRoughnessSq);

	float GGX = GGXV + GGXL;
	if (GGX > 0.0)
	{
		return 0.5 / GGX;
	}
	return 0.0;
}

// The following equation(s) model the distribution of microfacet normals across the area being drawn (aka D())
// Implementation from "Average Irregularity Representation of a Roughened Surface for Ray Reflection" by T. S. Trowbridge, and K. P. Reitz
// Follows the distribution function recommended in the SIGGRAPH 2013 course notes from EPIC Games [1], Equation 3.
float microfacetDistribution(in Surface surface, in SurfaceToLight surfaceToLight)
{
	float f = (surfaceToLight.NdotH * surface.alphaRoughnessSq - surfaceToLight.NdotH) * surfaceToLight.NdotH + 1.0;
	return surface.alphaRoughnessSq / (PI * f * f);
}


// Aniso functions source: https://github.com/google/filament/blob/main/shaders/src/brdf.fs

float D_GGX_Anisotropic(in Surface surface, in SurfaceToLight surfaceToLight)
{
	// Burley 2012, "Physically-Based Shading at Disney"

	// The values at and ab are perceptualRoughness^2, a2 is therefore perceptualRoughness^4
	// The dot product below computes perceptualRoughness^8. We cannot fit in fp16 without clamping
	// the roughness to too high values so we perform the dot product and the division in fp32
	float a2 = surface.at * surface.ab;
	float3 d = float3(surface.ab * surfaceToLight.TdotH, surface.at * surfaceToLight.BdotH, a2 * surfaceToLight.NdotH);
	float d2 = dot(d, d);
	float b2 = a2 / d2;
	return a2 * b2 * b2 / PI;
}
float V_SmithGGXCorrelated_Anisotropic(in Surface surface, in SurfaceToLight surfaceToLight)
{
	// Heitz 2014, "Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs"
	// TODO: lambdaV can be pre-computed for all the lights, it should be moved out of this function
	float lambdaV = surfaceToLight.NdotL * length(float3(surface.at * surface.TdotV, surface.ab * surface.BdotV, surface.NdotV));
	float lambdaL = surface.NdotV * length(float3(surface.at * surfaceToLight.TdotL, surface.ab * surfaceToLight.BdotL, surfaceToLight.NdotL));
	float v = 0.5 / (lambdaV + lambdaL);
	return saturate(v);
}


// These are the functions that will be used by shaders:
float3 BRDF_GetSpecular(in Surface surface, in SurfaceToLight surfaceToLight)
{
#ifdef BRDF_ANISOTROPIC
	float Vis = V_SmithGGXCorrelated_Anisotropic(surface, surfaceToLight);
	float D = D_GGX_Anisotropic(surface, surfaceToLight);
#else
	float Vis = visibilityOcclusion(surface, surfaceToLight);
	float D = microfacetDistribution(surface, surfaceToLight);
#endif // BRDF_ANISOTROPIC

	return surfaceToLight.F * Vis * D;
}
float3 BRDF_GetDiffuse(in Surface surface, in SurfaceToLight surfaceToLight)
{
	// Note: subsurface scattering will remove Fresnel (F), because otherwise
	//	there would be artifact on backside where diffuse wraps
	return (1 - lerp(surfaceToLight.F, 0, saturate(surface.sss.a))) / PI;
}

#endif // WI_BRDF_HF
