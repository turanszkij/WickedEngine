#include "globals.hlsli"
#include "ShaderInterop_Utility.h"

#ifndef MIP_OUTPUT_FORMAT
#define MIP_OUTPUT_FORMAT float4
#endif

TEXTURE3D(input, float4, TEXSLOT_UNIQUE0);
RWTEXTURE3D(input_output, MIP_OUTPUT_FORMAT, 0);

// Shader requires feature: Typed UAV additional format loads!
[numthreads(GENERATEMIPCHAIN_3D_BLOCK_SIZE, GENERATEMIPCHAIN_3D_BLOCK_SIZE, GENERATEMIPCHAIN_3D_BLOCK_SIZE)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	// Determine if the thread is alive (it is alive when the dispatchthreadID can directly index a pixel)
	if (DTid.x < outputResolution.x && DTid.y < outputResolution.y && DTid.z < outputResolution.z)
	{
		// Do a bilinear sample first and write it out:
		input_output[DTid] = input.SampleLevel(sampler_linear_clamp, ((float3)DTid + 0.5f) / (float3)outputResolution, 0);
		DeviceMemoryBarrier();

		uint i = 0;
		float4 sum = 0;

		// Gather samples in the X (horizontal) direction:
		[unroll]
		for (i = 0; i < 9; ++i)
		{
			sum += input_output[DTid + uint3(gaussianOffsets[i], 0, 0)] * gaussianWeightsNormalized[i];
		}
		// Write out the result of the horizontal blur:
		DeviceMemoryBarrier();
		input_output[DTid] = sum;
		DeviceMemoryBarrier();
		sum = 0;

		// Gather samples in the Y (vertical) direction:
		[unroll]
		for (i = 0; i < 9; ++i)
		{
			sum += input_output[DTid + uint3(0, gaussianOffsets[i], 0)] * gaussianWeightsNormalized[i];
		}
		// Write out the result of the vertical blur:
		DeviceMemoryBarrier();
		input_output[DTid] = sum;
		DeviceMemoryBarrier();
		sum = 0;

		// Gather samples in the Z (depth) direction:
		[unroll]
		for (i = 0; i < 9; ++i)
		{
			sum += input_output[DTid + uint3(0, 0, gaussianOffsets[i])] * gaussianWeightsNormalized[i];
		}
		// Write out the result of the depth blur:
		DeviceMemoryBarrier();
		input_output[DTid] = sum;
	}

}