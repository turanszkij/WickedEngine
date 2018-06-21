#include "globals.hlsli"
#include "ShaderInterop_Utility.h"

TEXTURE2D(input, float4, TEXSLOT_ONDEMAND0);

RWTEXTURE2D(output, unorm float4, 0);

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

	// wrapped borders:

	// left
	if (readcoord.x == 0)
	{
		output[writecoord + int2(-1, 0)] = input.Load(int3(readcoord + int2(xCopySrcSize.x - 1, 0), xCopySrcMIP));

		// top left
		if (readcoord.y == 0)
		{
			output[writecoord + int2(-1, -1)] = input.Load(int3(readcoord + int2(xCopySrcSize.x - 1, xCopySrcSize.x - 1), xCopySrcMIP));
		}
	}
	// right
	if (readcoord.x == xCopySrcSize.x - 1)
	{
		output[writecoord + int2(1, 0)] = input.Load(int3(readcoord + int2(-xCopySrcSize.x + 1, 0), xCopySrcMIP));

		// bottom right
		if (readcoord.y == xCopySrcSize.y - 1)
		{
			output[writecoord + int2(1, 1)] = input.Load(int3(readcoord + int2(-xCopySrcSize.x + 1, -xCopySrcSize.x + 1), xCopySrcMIP));
		}
	}

	// top
	if (readcoord.y == 0)
	{
		output[writecoord + int2(0, -1)] = input.Load(int3(readcoord + int2(0, xCopySrcSize.x - 1), xCopySrcMIP));

		// top right
		if (readcoord.x == xCopySrcSize.x - 1)
		{
			output[writecoord + int2(1, -1)] = input.Load(int3(readcoord + int2(-xCopySrcSize.x + 1, xCopySrcSize.x - 1), xCopySrcMIP));
		}
	}
	// bottom
	if (readcoord.y == xCopySrcSize.y - 1)
	{
		output[writecoord + int2(0, 1)] = input.Load(int3(readcoord + int2(0, -xCopySrcSize.x + 1), xCopySrcMIP));

		// bottom left
		if (readcoord.x == 0)
		{
			output[writecoord + int2(-1, 1)] = input.Load(int3(readcoord + int2(xCopySrcSize.x - 1, -xCopySrcSize.x + 1), xCopySrcMIP));
		}
	}
}
