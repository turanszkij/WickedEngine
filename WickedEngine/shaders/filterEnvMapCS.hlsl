#include "globals.hlsli"
#include "ShaderInterop_Renderer.h"

PUSHCONSTANT(push, FilterEnvmapPushConstants);

// From "Real Shading in UnrealEngine 4" by Brian Karis
float3 ImportanceSampleGGX(float2 Xi, float Roughness, float3 N)
{
	float a = Roughness * Roughness;
	float Phi = 2 * PI * Xi.x;
	float CosTheta = sqrt((1 - Xi.y) / (1 + (a * a - 1) * Xi.y));
	float SinTheta = sqrt(1 - CosTheta * CosTheta);
	float3 H;
	H.x = SinTheta * cos(Phi);
	H.y = SinTheta * sin(Phi); 
	H.z = CosTheta;
	return H;
}

[numthreads(GENERATEMIPCHAIN_2D_BLOCK_SIZE, GENERATEMIPCHAIN_2D_BLOCK_SIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	if (DTid.x < push.filterResolution.x && DTid.y < push.filterResolution.y)
	{
		TextureCube input = bindless_cubemaps[push.texture_input];
		RWTexture2DArray<float4> output = bindless_rwtextures2DArray[push.texture_output];

		float2 uv = (DTid.xy + 0.5f) * push.filterResolution_rcp.xy;
		float3 N = uv_to_cubemap(uv, DTid.z);

		float3x3 tangentSpace = get_tangentspace(N);

		float4 col = 0;

		for (uint i = 0; i < push.filterRayCount; ++i)
		{
			float2 hamm = hammersley2d(i, push.filterRayCount);
			float3 hemisphere = ImportanceSampleGGX(hamm, push.filterRoughness, N);
			float3 cone = mul(hemisphere, tangentSpace);

			col += input.SampleLevel(sampler_linear_clamp, cone, 0);
		}
		col /= (float)push.filterRayCount;

		output[uint3(DTid.xy, DTid.z)] = col;
	}
}
