#include "globals.hlsli"
// Just calculate luminance into power of two texture

RWTEXTURE2D(tex, float, 0);

[numthreads(16, 16, 1)]
void main(
	uint3 groupId : SV_GroupID,
	uint3 groupThreadId : SV_GroupThreadID,
	uint3 dispatchThreadId : SV_DispatchThreadID, // 
	uint groupIndex : SV_GroupIndex)
{
	float2 dimOut;
	tex.GetDimensions(dimOut.x, dimOut.y);
	float4 color = texture_0.SampleLevel(sampler_linear_clamp, (dispatchThreadId.xy + 0.5f) / dimOut, 0);
	tex[dispatchThreadId.xy] = max(0, dot(color.rgb, float3(0.2126, 0.7152, 0.0722)));
}
