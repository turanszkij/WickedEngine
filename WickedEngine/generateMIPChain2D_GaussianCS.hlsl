#include "globals.hlsli"
#include "generateMIPChainHF.hlsli"

globallycoherent RWTEXTURE2D(input_output, float4, 0);

// Shader requires feature: Typed UAV additional loads!
[numthreads(GENERATEMIPCHAIN_2D_BLOCK_SIZE, GENERATEMIPCHAIN_2D_BLOCK_SIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	uint2 dim;
	input_output.GetDimensions(dim.x, dim.y);

	if (DTid.x < dim.x && DTid.y < dim.y)
	{
		uint i = 0;
		float4 sum = 0;

		// Sum along X axis:
		[unroll]
		for (i = 0; i < 9; ++i)
		{
			sum += input_output[DTid.xy + uint2(gaussianOffsets[i], 0)] * gaussianWeightsNormalized[i];
		}
		input_output[DTid.xy] = sum;
		DeviceMemoryBarrier(); // globalycoherent flush!
		sum = 0;

		// Sum along Y axis:
		[unroll]
		for (i = 0; i < 9; ++i)
		{
			sum += input_output[DTid.xy + uint2(0, gaussianOffsets[i])] * gaussianWeightsNormalized[i];
		}
		input_output[DTid.xy] = sum;
	}
}