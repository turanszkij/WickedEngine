#include "globals.hlsli"
#include "ShaderInterop_Renderer.h"

PUSHCONSTANT(push, FilterEnvmapPushConstants);

// From "Real Shading in UnrealEngine 4" by Brian Karis, page 4
//	https://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf
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
	float3 UpVector = abs(N.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
	float3 TangentX = normalize(cross(UpVector, N));
	float3 TangentY = cross(N, TangentX);
	// Tangent to world space
	return TangentX * H.x + TangentY * H.y + N * H.z;
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
		float3 V = N;

		float4 col = 0;
		
		float Roughness = push.filterRoughness;

		uint rayCount = push.filterRayCount;
		for (uint i = 0; i < rayCount; ++i)
		{
			float2 Xi = hammersley2d(i, rayCount);
			float3 H = ImportanceSampleGGX(Xi, Roughness, N);
			float3 L = 2 * dot(V, H) * H - V;
			
			float NoL = saturate(dot(N, L));
			if (NoL > 0)
			{
				col += input.SampleLevel(sampler_linear_clamp, L, 0) * NoL;
			}
		}

		if(col.a > 0)
		{
			col /= col.a;
		}

		output[uint3(DTid.xy, DTid.z)] = col;
	}
}
