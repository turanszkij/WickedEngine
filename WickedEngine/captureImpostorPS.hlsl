#include "objectHF.hlsli"

struct ImpostorOut
{
	float4 color		: SV_Target0;
	float4 normal		: SV_Target1;
	float4 roughness	: SV_Target2;
	float4 reflectance	: SV_Target3;
	float4 metalness	: SV_Target4;
};

ImpostorOut main(PixelInputType input)
{
	float2 UV = input.tex * g_xMat_texMulAdd.xy + g_xMat_texMulAdd.zw;

	float3 N = normalize(input.nor);
	float3 P = input.pos3D;

	float3 T, B;
	float3x3 TBN = compute_tangent_frame(N, P, UV, T, B);

	float4 color = g_xMat_baseColor * float4(input.instanceColor, 1) * xBaseColorMap.Sample(sampler_objectshader, UV);

	float3 bumpColor;
	NormalMapping(UV, P, N, TBN, bumpColor);

	ImpostorOut Out = (ImpostorOut)0;
	Out.color = color;
	Out.normal = float4(mul(N, transpose(TBN)), 1);
	Out.roughness = g_xMat_roughness * xRoughnessMap.Sample(sampler_objectshader, UV).r;
	Out.reflectance = g_xMat_reflectance * xReflectanceMap.Sample(sampler_objectshader, UV).r;
	Out.metalness = g_xMat_metalness * xMetalnessMap.Sample(sampler_objectshader, UV).r;
	return Out;
}

