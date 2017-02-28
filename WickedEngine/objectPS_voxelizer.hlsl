#include "objectHF.hlsli"

struct VoxelOut
{
	float4 emission : SV_TARGET0;
	float4 normal	: SV_TARGET1;
};

VoxelOut main(float4 pos : SV_POSITION, float3 nor : NORMAL, float2 tex : TEXCOORD, uint RTIndex : SV_RenderTargetArrayIndex)
{
	VoxelOut Out;

	Out.emission = DEGAMMA(xBaseColorMap.Sample(sampler_linear_clamp, tex));
	Out.normal = float4(nor, 1);

	return Out;
}