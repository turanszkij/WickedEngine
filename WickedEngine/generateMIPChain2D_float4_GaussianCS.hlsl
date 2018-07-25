#include "globals.hlsli"
#include "generateMIPChainHF.hlsli"

#ifndef MIP_OUTPUT_FORMAT
#define MIP_OUTPUT_FORMAT float4
#endif

TEXTURE2D(input, float4, TEXSLOT_UNIQUE0);
RWTEXTURE2D(input_output, MIP_OUTPUT_FORMAT, 0);

// Shader requires feature: Typed UAV additional format loads!
[numthreads(GENERATEMIPCHAIN_2D_BLOCK_SIZE, GENERATEMIPCHAIN_2D_BLOCK_SIZE, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
#ifndef SHADERCOMPILER_SPIRV

	// Determine if the thread is alive (it is alive when the dispatchthreadID can directly index a pixel)
	if (DTid.x < outputResolution.x && DTid.y < outputResolution.y)
	{
		// Do a bilinear sample first and write it out:
		input_output[DTid.xy] = input.SampleLevel(sampler_linear_clamp, ((float2)DTid + 0.5f) / (float2)outputResolution.xy, 0);
		DeviceMemoryBarrier();

		uint i = 0;
		float4 sum = 0;

		// Gather samples in the X (horizontal) direction:
		[unroll]
		for (i = 0; i < 9; ++i)
		{
			sum += input_output[DTid.xy + uint2(gaussianOffsets[i], 0)] * gaussianWeightsNormalized[i];
		}
		// Write out the result of the horizontal blur:
		DeviceMemoryBarrier();
		input_output[DTid.xy] = sum;
		DeviceMemoryBarrier();
		sum = 0;

		// Gather samples in the Y (vertical) direction:
		[unroll]
		for (i = 0; i < 9; ++i)
		{
			sum += input_output[DTid.xy + uint2(0, gaussianOffsets[i])] * gaussianWeightsNormalized[i];
		}
		// Write out the result of the vertical blur:
		DeviceMemoryBarrier();
		input_output[DTid.xy] = sum;
	}

#endif
}