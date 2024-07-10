#include "globals.hlsli"
#include "ShaderInterop_Ocean.h"

Texture2D<float4> texture_displacementmap : register(t0);

RWTexture2D<float4> output : register(u0);

[numthreads(OCEAN_COMPUTE_TILESIZE, OCEAN_COMPUTE_TILESIZE, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	// Sample neighbour texels
	float2 one_texel = rcp(float2(xOceanOutWidth, xOceanOutHeight));
	float2 uv = ((float2)DTid.xy + 0.5) * one_texel;

	float2 tc_left = float2(uv.x - one_texel.x, uv.y);
	float2 tc_right = float2(uv.x + one_texel.x, uv.y);
	float2 tc_back = float2(uv.x, uv.y - one_texel.y);
	float2 tc_front = float2(uv.x, uv.y + one_texel.y);

	float3 displace_left = texture_displacementmap.SampleLevel(sampler_linear_wrap, tc_left, 0).xyz;
	float3 displace_right = texture_displacementmap.SampleLevel(sampler_linear_wrap, tc_right, 0).xyz;
	float3 displace_back = texture_displacementmap.SampleLevel(sampler_linear_wrap, tc_back, 0).xyz;
	float3 displace_front = texture_displacementmap.SampleLevel(sampler_linear_wrap, tc_front, 0).xyz;

	// Do not store the actual normal value. Using gradient instead, which preserves two differential values.
	float2 gradient = { -(displace_right.z - displace_left.z), -(displace_front.z - displace_back.z) };

	// Calculate Jacobian corelation from the partial differential of height field
	float2 Dx = (displace_right.xy - displace_left.xy) * xOceanChoppyScale * xOceanGridLen;
	float2 Dy = (displace_front.xy - displace_back.xy) * xOceanChoppyScale * xOceanGridLen;
	float J = (1.0f + Dx.x) * (1.0f + Dy.y) - Dx.y * Dy.x;

	// Practical subsurface scale calculation: max[0, (1 - J) + Amplitude * (2 * Coverage - 1)].
	float fold = max(1.0f - J, 0);

	// Output
	output[DTid.xy] = float4(gradient, 0, fold);
}
