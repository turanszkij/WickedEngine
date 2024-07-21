#include "globals.hlsli"

RWTexture2D<float4> output : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	uint2 dim;
	output.GetDimensions(dim.x, dim.y);
	float2 dim_rcp = rcp(dim);
	float2 uv = DTid.xy * dim_rcp * 10;
	float time = GetTime();
	float4 caustics = float4(
		caustic_pattern(uv + float2(0, 0), time),
		caustic_pattern(uv + float2(dim_rcp.x * 8, 0), time),
		caustic_pattern(uv + float2(0, dim_rcp.y * 8), time),
		1
	);
	output[DTid.xy] = caustics;
}
