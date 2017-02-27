#include "globals.hlsli"

TEXTURE3D(input_emittance, float4, 0);
TEXTURE3D(input_normal, float3, 1);
RWTEXTURE3D(output, float4, 0);

groupshared float4 accumulation[4 * 4 * 4];

static const float3 SAMPLES[16] = {
	float3(0.355512, -0.709318, -0.102371),
	float3(0.534186, 0.71511, -0.115167),
	float3(-0.87866, 0.157139, -0.115167),
	float3(0.140679, -0.475516, -0.0639818),
	float3(-0.0796121, 0.158842, -0.677075),
	float3(-0.0759516, -0.101676, -0.483625),
	float3(0.12493, -0.0223423, -0.483625),
	float3(-0.0720074, 0.243395, -0.967251),
	float3(-0.207641, 0.414286, 0.187755),
	float3(-0.277332, -0.371262, 0.187755),
	float3(0.63864, -0.114214, 0.262857),
	float3(-0.184051, 0.622119, 0.262857),
	float3(0.110007, -0.219486, 0.435574),
	float3(0.235085, 0.314707, 0.696918),
	float3(-0.290012, 0.0518654, 0.522688),
	float3(0.0975089, -0.329594, 0.609803)
};

[numthreads(4, 4, 4)]
void main( uint3 DTid : SV_DispatchThreadID, uint GroupIndex : SV_GroupIndex )
{
	float4 emittance = input_emittance[DTid];
	float3 normal = input_normal[DTid];

	output[DTid] = emittance;
	//output[DTid] = float4(normal.rgb * 0.5f + 0.5f, emittance.a);

	//float3 dim;
	//input.GetDimensions(dim.x, dim.y, dim.z);
	//float3 uvw = DTid / dim;

	//float occ = 0;
	//uint count = 0;
	//for (uint i = 0; i < 16; ++i)
	//{
	//	for (uint j = 1; j < 6; ++j)
	//	{
	//		float3 tex = uvw + j * SAMPLES[i] / dim;
	//		occ += input.SampleLevel(sampler_linear_clamp, tex, 0).a;

	//		count++;
	//	}
	//}

	//output[DTid] = occ / count;
}