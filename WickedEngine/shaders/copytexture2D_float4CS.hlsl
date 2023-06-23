#include "globals.hlsli"
#include "ShaderInterop_Renderer.h"
#include "ColorSpaceUtility.hlsli"

#ifndef COPY_OUTPUT_FORMAT
#define COPY_OUTPUT_FORMAT float4
#endif // COPY_OUTPUT_FORMAT

PUSHCONSTANT(push, CopyTextureCB);

Texture2D<float4> input : register(t0);

RWTexture2D<COPY_OUTPUT_FORMAT> output : register(u0);

#define CONVERT(x) ((push.xCopyFlags & COPY_TEXTURE_SRGB) ? float4(ApplySRGBCurve(x.rgb), x.a) : (x))

[numthreads(8, 8, 1)]
void main(int3 DTid : SV_DispatchThreadID)
{
	const int2 readcoord = DTid.xy + push.xCopySrc;
	const int2 writecoord = DTid.xy + push.xCopyDst;

	output[writecoord] = CONVERT(input.Load(int3(readcoord, push.xCopySrcMIP)));
}
