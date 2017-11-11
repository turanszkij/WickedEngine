#include "oceanWaveGenHF.hlsli"

// Displacement -> Normal, Folding
float4 main(VS_QUAD_OUTPUT In) : SV_Target
{
	// Sample neighbour texels
	float2 one_texel = float2(1.0f / (float)g_OutWidth, 1.0f / (float)g_OutHeight);

	float2 tc_left = float2(In.TexCoord.x - one_texel.x, In.TexCoord.y);
	float2 tc_right = float2(In.TexCoord.x + one_texel.x, In.TexCoord.y);
	float2 tc_back = float2(In.TexCoord.x, In.TexCoord.y - one_texel.y);
	float2 tc_front = float2(In.TexCoord.x, In.TexCoord.y + one_texel.y);

	float3 displace_left = g_samplerDisplacementMap.Sample(LinearSampler, tc_left).xyz;
	float3 displace_right = g_samplerDisplacementMap.Sample(LinearSampler, tc_right).xyz;
	float3 displace_back = g_samplerDisplacementMap.Sample(LinearSampler, tc_back).xyz;
	float3 displace_front = g_samplerDisplacementMap.Sample(LinearSampler, tc_front).xyz;

	// Do not store the actual normal value. Using gradient instead, which preserves two differential values.
	float2 gradient = { -(displace_right.z - displace_left.z), -(displace_front.z - displace_back.z) };


	// Calculate Jacobian corelation from the partial differential of height field
	float2 Dx = (displace_right.xy - displace_left.xy) * g_ChoppyScale * g_GridLen;
	float2 Dy = (displace_front.xy - displace_back.xy) * g_ChoppyScale * g_GridLen;
	float J = (1.0f + Dx.x) * (1.0f + Dy.y) - Dx.y * Dy.x;

	// Practical subsurface scale calculation: max[0, (1 - J) + Amplitude * (2 * Coverage - 1)].
	float fold = max(1.0f - J, 0);

	// Output
	return float4(gradient, 0, fold);
}
