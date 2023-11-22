#include "globals.hlsli"
#include "ShaderInterop_Renderer.h"

PUSHCONSTANT(push, PaintTextureCB);

Texture2D<float4> texture_brush : register(t0);
Texture2D<float4> texture_reveal : register(t1);

RWTexture2D<unorm float4> output : register(u0);

[numthreads(PAINT_TEXTURE_BLOCKSIZE, PAINT_TEXTURE_BLOCKSIZE, 1)]
void main( uint3 DTid : SV_DispatchThreadID )
{
	float2 dim;
	output.GetDimensions(dim.x, dim.y);

	const uint2 pixelUnwrapped = push.xPaintBrushCenter + DTid.xy - push.xPaintBrushRadius.xx;
	const uint2 pixel = pixelUnwrapped % dim.xy;

	const float2x2 rot = float2x2(
		cos(push.xPaintBrushRotation), -sin(push.xPaintBrushRotation),
		sin(push.xPaintBrushRotation), cos(push.xPaintBrushRotation)
		);
	const float2 diff = mul((float2)push.xPaintBrushCenter - (float2)pixelUnwrapped, rot);

	float dist = 0;
	float radius = (float)push.xPaintBrushRadius;
	switch (push.xPaintBrushShape)
	{
	default:
	case 0:
		dist = length(diff);
		break;
	case 1:
		dist = max(abs(diff.x), abs(diff.y));
		radius /= SQRT2;
		break;
	}

	const float2 brush_uv = (diff / radius) * float2(0.5, -0.5) + 0.5;
	const float2 brush_uv_quad_x = QuadReadAcrossX(brush_uv);
	const float2 brush_uv_quad_y = QuadReadAcrossY(brush_uv);
	const float2 brush_uv_dx = brush_uv - brush_uv_quad_x;
	const float2 brush_uv_dy = brush_uv - brush_uv_quad_y;
	float4 brush_color = texture_brush.SampleGrad(sampler_linear_clamp, brush_uv, brush_uv_dx, brush_uv_dy) * unpack_rgba(push.xPaintBrushColor);

	const float2 reveal_uv = (pixel + 0.5) / dim;
	const float2 reveal_uv_quad_x = QuadReadAcrossX(reveal_uv);
	const float2 reveal_uv_quad_y = QuadReadAcrossY(reveal_uv);
	const float2 reveal_uv_dx = reveal_uv - reveal_uv_quad_x;
	const float2 reveal_uv_dy = reveal_uv - reveal_uv_quad_y;
	float4 reveal_color = texture_reveal.SampleGrad(sampler_linear_clamp, reveal_uv, reveal_uv_dx, reveal_uv_dy);

	const float affection = push.xPaintBrushAmount * smoothstep(0, push.xPaintBrushSmoothness, 1 - dist / radius);
	if (affection > 0 && dist < radius)
	{
		if (push.xPaintReveal)
		{
			brush_color *= reveal_color;
		}
		output[pixel] = lerp(output[pixel], brush_color, affection);
	}
	//else
	//{
	//	output[pixel] = float4(1,0,0,1);
	//}
}
