#include "globals.hlsli"
// Resolve MSAA depth buffer to a non MSAA texture

TEXTURE2DMS(input, float, TEXSLOT_ONDEMAND0);
RWTEXTURE2D(output, float, 0);

[numthreads(8, 8, 1)]
void main(uint3 dispatchThreadId : SV_DispatchThreadID)
{
	uint2 dim;
	uint sampleCount;
	input.GetDimensions(dim.x, dim.y, sampleCount);
	if (dispatchThreadId.x > dim.x || dispatchThreadId.y > dim.y)
	{
		return;
	}

	float result = 0;
	for (uint i = 0; i < sampleCount; ++i)
	{
		result = max(result, input.Load(dispatchThreadId.xy, i).r);
	}

	output[dispatchThreadId.xy] = result;
}
