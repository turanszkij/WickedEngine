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

	// border expansion:
	const bool wrap = xCopyBorderExpandStyle == 1;
	int2 readoffset;

	// left
	if (readcoord.x == 0)
	{
		readoffset = wrap ? int2(xCopySrcSize.x - 1, 0) : 0;

		output[writecoord + int2(-1, 0)] = input.Load(int3(readcoord + readoffset, xCopySrcMIP));

		// top left
		if (readcoord.y == 0)
		{
			readoffset = wrap ? int2(xCopySrcSize.x - 1, xCopySrcSize.x - 1) : 0;

			output[writecoord + int2(-1, -1)] = input.Load(int3(readcoord + readoffset, xCopySrcMIP));
		}
	}
	// right
	if (readcoord.x == xCopySrcSize.x - 1)
	{
		readoffset = wrap ? int2(-xCopySrcSize.x + 1, 0) : 0;

		output[writecoord + int2(1, 0)] = input.Load(int3(readcoord + readoffset, xCopySrcMIP));

		// bottom right
		if (readcoord.y == xCopySrcSize.y - 1)
		{
			readoffset = wrap ? int2(-xCopySrcSize.x + 1, -xCopySrcSize.x + 1) : 0;

			output[writecoord + int2(1, 1)] = input.Load(int3(readcoord + readoffset, xCopySrcMIP));
		}
	}

	// top
	if (readcoord.y == 0)
	{
		readoffset = wrap ? int2(0, xCopySrcSize.x - 1) : 0;

		output[writecoord + int2(0, -1)] = input.Load(int3(readcoord + readoffset, xCopySrcMIP));

		// top right
		if (readcoord.x == xCopySrcSize.x - 1)
		{
			readoffset = wrap ? int2(-xCopySrcSize.x + 1, xCopySrcSize.x - 1) : 0;

			output[writecoord + int2(1, -1)] = input.Load(int3(readcoord + readoffset, xCopySrcMIP));
		}
	}
	// bottom
	if (readcoord.y == xCopySrcSize.y - 1)
	{
		readoffset = wrap ? int2(0, -xCopySrcSize.x + 1) : 0;

		output[writecoord + int2(0, 1)] = input.Load(int3(readcoord + readoffset, xCopySrcMIP));

		// bottom left
		if (readcoord.x == 0)
		{
			readoffset = wrap ? int2(xCopySrcSize.x - 1, -xCopySrcSize.x + 1) : 0;

			output[writecoord + int2(-1, 1)] = input.Load(int3(readcoord + readoffset, xCopySrcMIP));
		}
	}
}
