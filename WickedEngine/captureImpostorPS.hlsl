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
	OBJECT_PS_MAKE_COMMON

	OBJECT_PS_SAMPLETEXTURES

	OBJECT_PS_EMISSIVE

	OBJECT_PS_COMPUTETANGENTSPACE

	OBJECT_PS_NORMALMAPPING

	ImpostorOut Out = (ImpostorOut)0;
	Out.color = color;
	Out.normal = float4(mul(N, transpose(TBN)), 1);
	Out.roughness = roughness;
	Out.reflectance = reflectance;
	Out.metalness = metalness;
	return Out;
}

