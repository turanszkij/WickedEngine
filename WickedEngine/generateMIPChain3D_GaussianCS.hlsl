#include "globals.hlsli"
#include "generateMIPChainHF.hlsli"

globallycoherent RWTEXTURE3D(input_output, float4, 0);

// Shader requires feature: Typed UAV additional loads!
[numthreads(GENERATEMIPCHAIN_3D_BLOCK_SIZE, GENERATEMIPCHAIN_3D_BLOCK_SIZE, GENERATEMIPCHAIN_3D_BLOCK_SIZE)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	uint3 dim;
	input_output.GetDimensions(dim.x, dim.y, dim.z);

	if (DTid.x < dim.x && DTid.y < dim.y && DTid.z < dim.z)
	{
		uint i = 0;
		float4 sum = 0;

		// Sum along X axis:
		[unroll]
		for (i = 0; i < 9; ++i)
		{
			sum += input_output[DTid + uint3(gaussianOffsets[i], 0, 0)] * gaussianWeightsNormalized[i];
		}
		input_output[DTid] = sum;
		DeviceMemoryBarrier(); // globalycoherent flush!
		sum = 0;

		// Sum along Y axis:
		[unroll]
		for (i = 0; i < 9; ++i)
		{
			sum += input_output[DTid + uint3(0, gaussianOffsets[i], 0)] * gaussianWeightsNormalized[i];
		}
		input_output[DTid] = sum;
		DeviceMemoryBarrier(); // globalycoherent flush!
		sum = 0;

		// Sum along Z axis:
		[unroll]
		for (i = 0; i < 9; ++i)
		{
			sum += input_output[DTid + uint3(0, 0, gaussianOffsets[i])] * gaussianWeightsNormalized[i];
		}
		input_output[DTid] = sum;
	}
}