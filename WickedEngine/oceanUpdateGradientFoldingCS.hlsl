#include "globals.hlsli"
#include "ShaderInterop_Ocean.h"

TEXTURE2D(texture_displacementmap, float4, TEXSLOT_ONDEMAND0);

RWTEXTURE2D(output, float4, 0);

[numthreads(OCEAN_COMPUTE_TILESIZE, OCEAN_COMPUTE_TILESIZE, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	// Sample neighbour texels
	float2 one_texel = float2(1.0f / (float)g_OutWidth, 1.0f / (float)g_OutHeight);

	float2 uv = (float2)DTid.xy / float2(g_OutWidth, g_OutHeight);

	float2 tc_left = float2(uv.x - one_texel.x, uv.y);
	float2 tc_right = float2(uv.x + one_texel.x, uv.y);
	float2 tc_back = float2(uv.x, uv.y - one_texel.y);
	float2 tc_front = float2(uv.x, uv.y + one_texel.y);

	float3 displace_left = texture_displacementmap.SampleLevel(sampler_linear_clamp, tc_left, 0).xyz;
	float3 displace_right = texture_displacementmap.SampleLevel(sampler_linear_clamp, tc_right, 0).xyz;
	float3 displace_back = texture_displacementmap.SampleLevel(sampler_linear_clamp, tc_back, 0).xyz;
	float3 displace_front = texture_displacementmap.SampleLevel(sampler_linear_clamp, tc_front, 0).xyz;

	// Do not store the actual normal value. Using gradient instead, which preserves two differential values.
	float2 gradient = { -(displace_right.z - displace_left.z), -(displace_front.z - displace_back.z) };


	// Calculate Jacobian corelation from the partial differential of height field
	float2 Dx = (displace_right.xy - displace_left.xy) * g_ChoppyScale * g_GridLen;
	float2 Dy = (displace_front.xy - displace_back.xy) * g_ChoppyScale * g_GridLen;
	float J = (1.0f + Dx.x) * (1.0f + Dy.y) - Dx.y * Dy.x;

	// Practical subsurface scale calculation: max[0, (1 - J) + Amplitude * (2 * Coverage - 1)].
	float fold = max(1.0f - J, 0);

	// Output
	output[DTid.xy] = float4(gradient, 0, fold);
}