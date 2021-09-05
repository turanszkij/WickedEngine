#include "globals.hlsli"
#include "ShaderInterop_Renderer.h"

#ifndef COPY_OUTPUT_FORMAT
#define COPY_OUTPUT_FORMAT float4
#endif // COPY_OUTPUT_FORMAT

PUSHCONSTANT(push, CopyTextureCB);

TEXTURE2D(input, float4, TEXSLOT_ONDEMAND0);

RWTEXTURE2D(output, COPY_OUTPUT_FORMAT, 0);

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
}
