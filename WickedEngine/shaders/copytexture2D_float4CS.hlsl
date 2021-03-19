#include "globals.hlsli"
#include "ShaderInterop_Utility.h"

#ifndef COPY_OUTPUT_FORMAT
#define COPY_OUTPUT_FORMAT float4
#endif // COPY_OUTPUT_FORMAT

TEXTURE2D(input, float4, TEXSLOT_ONDEMAND0);

RWTEXTURE2D(output, COPY_OUTPUT_FORMAT, 0);

[numthreads(8, 8, 1)]
void main(int3 DTid : SV_DispatchThreadID)
{
	if (DTid.x >= xCopySrcSize.x || DTid.y >= xCopySrcSize.y)
	{
		return;
	}

	const int2 readcoord = DTid.xy;
	const int2 writecoord = DTid.xy + xCopyDest;

	output[writecoord] = input.Load(int3(readcoord, xCopySrcMIP));
}
