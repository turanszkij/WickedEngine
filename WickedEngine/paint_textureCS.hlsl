#include "globals.hlsli"
#include "ShaderInterop_Paint.h"

TEXTURE2D(texture_brush, float4, TEXSLOT_ONDEMAND0);

RWTEXTURE2D(output, unorm float4, 0);

[numthreads(PAINT_TEXTURE_BLOCKSIZE, PAINT_TEXTURE_BLOCKSIZE, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	const uint2 pixel = xPaintBrushCenter + DTid.xy - xPaintBrushRadius.xx;
	const float2 uv = (DTid.xy + 0.5f) / xPaintBrushRadius.xx;
	const float4 color = texture_brush.SampleLevel(sampler_linear_clamp, uv, 0) * unpack_rgba(xPaintBrushColor);
	const float dist = distance((float2)pixel, (float2)xPaintBrushCenter);
	const float affection = xPaintBrushAmount * pow(1 - saturate(dist / (float)xPaintBrushRadius), xPaintBrushFalloff);
	if (affection > 0 && dist < (float)xPaintBrushRadius)
	{
		output[pixel] = lerp(output[pixel], color, affection);
	}
}
