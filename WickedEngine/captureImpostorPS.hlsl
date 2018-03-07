#include "objectHF.hlsli"

struct ImpostorOut
{
	float4 color		: SV_Target0;
	float4 normal		: SV_Target1;
	float4 surfaceProps	: SV_Target2;
};

ImpostorOut main(PixelInputType input)
{
	float2 UV = input.tex * g_xMat_texMulAdd.xy + g_xMat_texMulAdd.zw;

	float3 N = normalize(input.nor);
	float3 P = input.pos3D;

	float3 T, B;
	float3x3 TBN = compute_tangent_frame(N, P, UV, T, B);

	float4 color = g_xMat_baseColor * float4(input.instanceColor, 1) * xBaseColorMap.Sample(sampler_objectshader, UV);
	ALPHATEST(color.a);
	color.a = 1;

	float roughness = g_xMat_roughness;

	float3 bumpColor;
	NormalMapping(UV, P, N, TBN, bumpColor, roughness);

	float4 surface_ref_met_emi_sss = xSurfaceMap.Sample(sampler_objectshader, UV);

	ImpostorOut Out;
	Out.color = color;
	Out.normal = float4(mul(N, transpose(TBN)), roughness);
	Out.surfaceProps.r = g_xMat_reflectance * surface_ref_met_emi_sss.r;
	Out.surfaceProps.g = g_xMat_metalness * surface_ref_met_emi_sss.g;
	Out.surfaceProps.b = g_xMat_emissive * surface_ref_met_emi_sss.b;
	Out.surfaceProps.a = g_xMat_subsurfaceScattering * surface_ref_met_emi_sss.a;
	return Out;
}

