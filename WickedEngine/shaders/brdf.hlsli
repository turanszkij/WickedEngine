#ifndef WI_BRDF_HF
#define WI_BRDF_HF
#include "globals.hlsli"
#include "surfaceHF.hlsli"

// BRDF functions source: https://github.com/google/filament/blob/main/shaders/src/brdf.fs

#define MEDIUMP_FLT_MAX    65504.0
#define saturateMediump(x) min(x, MEDIUMP_FLT_MAX)
#define highp

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

float iorToF0(float transmittedIor, float incidentIor)
{
	return sqr((transmittedIor - incidentIor) / (transmittedIor + incidentIor));
}

float f0ToIor(float f0)
{
	float r = sqrt(f0);
	return (1.0 + r) / (1.0 - r);
}

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

#ifdef ANISOTROPIC
	float TdotL;
	float BdotL;
	float TdotH;
	float BdotH;
#endif // ANISOTROPIC

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

		F = F_Schlick(surface.f0, VdotH);

#ifdef ANISOTROPIC
		TdotL = dot(surface.aniso.T.xyz, L);
		BdotL = dot(surface.aniso.B, L);
		TdotH = dot(surface.aniso.T.xyz, H);
		BdotH = dot(surface.aniso.B, H);
#endif // ANISOTROPIC

#ifdef CARTOON
		// SSS is handled differently in cartoon shader:
		//	1) The diffuse wraparound is monochrome at first to avoid banding with smoothstep()
		NdotL_sss = (NdotL + surface.sss.a) * surface.sss_inv.a;

		NdotL = smoothstep(0.005, 0.05, NdotL);
		NdotL_sss = smoothstep(0.005, 0.05, NdotL_sss);
		NdotH = smoothstep(0.98, 0.99, NdotH);

		// SSS is handled differently in cartoon shader:
		//	2) The diffuse wraparound is tinted after smoothstep
		NdotL_sss = (NdotL_sss + surface.sss.rgb) * surface.sss_inv.rgb;
#endif // CARTOON

		NdotL = saturate(NdotL);
		NdotL_sss = saturate(NdotL_sss);
	}
};


// These are the functions that will be used by shaders:

float3 BRDF_GetSpecular(in Surface surface, in SurfaceToLight surface_to_light)
{
#ifdef ANISOTROPIC
	float D = D_GGX_Anisotropic(surface.aniso.at, surface.aniso.ab, surface_to_light.TdotH, surface_to_light.BdotH, surface_to_light.NdotH);
	float Vis = V_SmithGGXCorrelated_Anisotropic(surface.aniso.at, surface.aniso.ab, surface.aniso.TdotV, surface.aniso.BdotV,
		surface_to_light.TdotL, surface_to_light.BdotL, surface.NdotV, surface_to_light.NdotL);
#else
	float roughnessBRDF = sqr(clamp(surface.roughness, 0.045, 1));
	float D = D_GGX(roughnessBRDF, surface_to_light.NdotH, surface_to_light.H);
	float Vis = V_SmithGGXCorrelated(roughnessBRDF, surface.NdotV, surface_to_light.NdotL);
#endif // ANISOTROPIC

	float3 specular = D * Vis * surface_to_light.F;

#ifdef SHEEN
	specular *= surface.sheen.albedoScaling;
	float sheen_roughnessBRDF = sqr(clamp(surface.sheen.roughness, 0.045, 1));
	D = D_Charlie(sheen_roughnessBRDF, surface_to_light.NdotH);
	Vis = V_Neubelt(surface.NdotV, surface_to_light.NdotL);
	specular += D * Vis * surface.sheen.color;
#endif // SHEEN

#ifdef CLEARCOAT
	specular *= 1 - surface.clearcoat.F;
	float NdotH = saturate(dot(surface.clearcoat.N, surface_to_light.H));
	float clearcoat_roughnessBRDF = sqr(clamp(surface.clearcoat.roughness, 0.045, 1));
	D = D_GGX(clearcoat_roughnessBRDF, NdotH, surface_to_light.H);
	Vis = V_Kelemen(surface_to_light.LdotH);
	specular += D * Vis * surface.clearcoat.F;
#endif // CLEARCOAT

	return specular * surface_to_light.NdotL;
}
float3 BRDF_GetDiffuse(in Surface surface, in SurfaceToLight surface_to_light)
{
	// Note: subsurface scattering will remove Fresnel (F), because otherwise
	//	there would be artifact on backside where diffuse wraps
	float3 diffuse = (1 - lerp(surface_to_light.F, 0, saturate(surface.sss.a)));

#ifdef SHEEN
	diffuse *= surface.sheen.albedoScaling;
#endif // SHEEN

#ifdef CLEARCOAT
	diffuse *= 1 - surface.clearcoat.F;
#endif // CLEARCOAT

	return diffuse * surface_to_light.NdotL_sss;
}

#endif // WI_BRDF_HF
