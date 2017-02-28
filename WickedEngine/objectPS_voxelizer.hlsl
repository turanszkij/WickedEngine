#include "objectHF.hlsli"

struct VoxelOut
{
	float4 emission : SV_TARGET0;
	float4 normal	: SV_TARGET1;
};

VoxelOut main(float4 pos : SV_POSITION, float3 nor : NORMAL, float2 tex : TEXCOORD/*, float3 pos3D : POSITION3D*/, uint RTIndex : SV_RenderTargetArrayIndex)
{
	float4 color = DEGAMMA(xBaseColorMap.Sample(sampler_linear_clamp, tex));


	VoxelOut Out;

	Out.emission = color;
	Out.normal = float4(nor, 1);

	return Out;
}