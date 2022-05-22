#include "globals.hlsli"
#include "ShaderInterop_Renderer.h"

PUSHCONSTANT(push, PaintTextureCB);

Texture2D<float4> texture_brush : register(t0);
Texture2D<float4> texture_reveal : register(t1);

RWTexture2D<unorm float4> output : register(u0);

[numthreads(PAINT_TEXTURE_BLOCKSIZE, PAINT_TEXTURE_BLOCKSIZE, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	const uint2 pixel = push.xPaintBrushCenter + DTid.xy - push.xPaintBrushRadius.xx;

	const float2 brush_uv = (DTid.xy + 0.5) / (push.xPaintBrushRadius.xx * 2);
	const float2 brush_uv_quad_x = QuadReadAcrossX(brush_uv);
	const float2 brush_uv_quad_y = QuadReadAcrossY(brush_uv);
	const float2 brush_uv_dx = brush_uv - brush_uv_quad_x;
	const float2 brush_uv_dy = brush_uv - brush_uv_quad_y;
	float4 brush_color = texture_brush.SampleGrad(sampler_linear_clamp, brush_uv, brush_uv_dx, brush_uv_dy) * unpack_rgba(push.xPaintBrushColor);

	float2 dim;
	output.GetDimensions(dim.x, dim.y);
	const float2 reveal_uv = (pixel + 0.5) / dim;
	const float2 reveal_uv_quad_x = QuadReadAcrossX(reveal_uv);
	const float2 reveal_uv_quad_y = QuadReadAcrossY(reveal_uv);
	const float2 reveal_uv_dx = reveal_uv - reveal_uv_quad_x;
	const float2 reveal_uv_dy = reveal_uv - reveal_uv_quad_y;
	float4 reveal_color = texture_reveal.SampleGrad(sampler_linear_clamp, reveal_uv, reveal_uv_dx, reveal_uv_dy);

	const float dist = distance((float2)pixel, (float2)push.xPaintBrushCenter);
	const float affection = push.xPaintBrushAmount * pow(1 - saturate(dist / (float)push.xPaintBrushRadius), push.xPaintBrushFalloff);
	if (affection > 0 && dist < (float)push.xPaintBrushRadius)
	{
		if (push.xPaintReveal)
		{
			brush_color *= reveal_color;
		}
		output[pixel] = lerp(output[pixel], brush_color, affection);
	}
}
