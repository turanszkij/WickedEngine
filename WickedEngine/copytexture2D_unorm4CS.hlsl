#include "globals.hlsli"
#include "ShaderInterop_Utility.h"

TEXTURE2D(input, float4, TEXSLOT_ONDEMAND0);

RWTEXTURE2D(output, unorm float4, 0);

[numthreads(8, 8, 1)]
void main( int3 DTid : SV_DispatchThreadID )
{
	if (DTid.x >= xCopySrcSize.x || DTid.y >= xCopySrcSize.y)
	{
		return;
	}

	const int2 readcoord = DTid.xy;
	const int2 writecoord = DTid.xy + xCopyDest;

	output[writecoord] = input.Load(int3(readcoord, xCopySrcMIP));
}
