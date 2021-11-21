#include "globals.hlsli"
#include "ShaderInterop_Renderer.h"

#ifndef COPY_OUTPUT_FORMAT
#define COPY_OUTPUT_FORMAT float4
#endif // COPY_OUTPUT_FORMAT

PUSHCONSTANT(push, CopyTextureCB);

Texture2D<float4> input : register(t0);

RWTexture2D<COPY_OUTPUT_FORMAT> output : register(u0);

[numthreads(8, 8, 1)]
void main(int3 DTid : SV_DispatchThreadID)
{
	if (DTid.x >= push.xCopySrcSize.x || DTid.y >= push.xCopySrcSize.y)
	{
		return;
	}

	const int2 readcoord = DTid.xy;
	const int2 writecoord = DTid.xy + push.xCopyDest;

	output[writecoord] = input.Load(int3(readcoord, push.xCopySrcMIP));

	// border expansion:
	const bool wrap = push.xCopyBorderExpandStyle == 1;
	int2 readoffset;

	// left
	if (readcoord.x == 0)
	{
		readoffset = wrap ? int2(push.xCopySrcSize.x - 1, 0) : 0;

		output[writecoord + int2(-1, 0)] = input.Load(int3(readcoord + readoffset, push.xCopySrcMIP));

		// top left
		if (readcoord.y == 0)
		{
			readoffset = wrap ? int2(push.xCopySrcSize.x - 1, push.xCopySrcSize.x - 1) : 0;

			output[writecoord + int2(-1, -1)] = input.Load(int3(readcoord + readoffset, push.xCopySrcMIP));
		}
	}
	// right
	if (readcoord.x == push.xCopySrcSize.x - 1)
	{
		readoffset = wrap ? int2(-push.xCopySrcSize.x + 1, 0) : 0;

		output[writecoord + int2(1, 0)] = input.Load(int3(readcoord + readoffset, push.xCopySrcMIP));

		// bottom right
		if (readcoord.y == push.xCopySrcSize.y - 1)
		{
			readoffset = wrap ? int2(-push.xCopySrcSize.x + 1, -push.xCopySrcSize.x + 1) : 0;

			output[writecoord + int2(1, 1)] = input.Load(int3(readcoord + readoffset, push.xCopySrcMIP));
		}
	}

	// top
	if (readcoord.y == 0)
	{
		readoffset = wrap ? int2(0, push.xCopySrcSize.x - 1) : 0;

		output[writecoord + int2(0, -1)] = input.Load(int3(readcoord + readoffset, push.xCopySrcMIP));

		// top right
		if (readcoord.x == push.xCopySrcSize.x - 1)
		{
			readoffset = wrap ? int2(-push.xCopySrcSize.x + 1, push.xCopySrcSize.x - 1) : 0;

			output[writecoord + int2(1, -1)] = input.Load(int3(readcoord + readoffset, push.xCopySrcMIP));
		}
	}
	// bottom
	if (readcoord.y == push.xCopySrcSize.y - 1)
	{
		readoffset = wrap ? int2(0, -push.xCopySrcSize.x + 1) : 0;

		output[writecoord + int2(0, 1)] = input.Load(int3(readcoord + readoffset, push.xCopySrcMIP));

		// bottom left
		if (readcoord.x == 0)
		{
			readoffset = wrap ? int2(push.xCopySrcSize.x - 1, -push.xCopySrcSize.x + 1) : 0;

			output[writecoord + int2(-1, 1)] = input.Load(int3(readcoord + readoffset, push.xCopySrcMIP));
		}
	}
}
