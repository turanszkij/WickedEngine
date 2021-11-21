#include "globals.hlsli"
#include "ShaderInterop_Renderer.h"

PUSHCONSTANT(push, PaintTextureCB);

Texture2D<float4> texture_brush : register(t0);

RWTexture2D<unorm float4> output : register(u0);

[numthreads(PAINT_TEXTURE_BLOCKSIZE, PAINT_TEXTURE_BLOCKSIZE, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	const uint2 pixel = push.xPaintBrushCenter + DTid.xy - push.xPaintBrushRadius.xx;
	const float2 uv = (DTid.xy + 0.5f) / push.xPaintBrushRadius.xx;
	const float4 color = texture_brush.SampleLevel(sampler_linear_clamp, uv, 0) * unpack_rgba(push.xPaintBrushColor);
	const float dist = distance((float2)pixel, (float2)push.xPaintBrushCenter);
	const float affection = push.xPaintBrushAmount * pow(1 - saturate(dist / (float)push.xPaintBrushRadius), push.xPaintBrushFalloff);
	if (affection > 0 && dist < (float)push.xPaintBrushRadius)
	{
		output[pixel] = lerp(output[pixel], color, affection);
	}
}
