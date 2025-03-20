#include "globals.hlsli"
// This shader is writing stencil with DepthStencilState and only discarding pixels
//	if bit is not contained in the stencilMask texture
//	With 8 full screen passes, it is possible to copy over all bits from stencilMask
//	texture into a real depth stencil buffer
//
//	Note: The simpler method is to use CopyTexture from ImageAspect::COLOR to ImageAspect::STENCIL
//	But Vulkan doesn't support that currently, so this is a workaround for that
//	Vulkan issue: https://github.com/KhronosGroup/Vulkan-Docs/issues/2079
//
//	Note: this is also used for scaling stencil, that's why it is working with UV coordinates

PUSHCONSTANT(push, StencilBitPush);

#ifdef MSAA

Texture2DMS<uint> input_stencil : register(t0);

void main(float4 pos : SV_Position, out uint coverage : SV_Coverage)
{
	const float2 uv = pos.xy * push.output_resolution_rcp;
	const uint2 input_resolution = uint2(push.input_resolution & 0xFFFF, push.input_resolution >> 16u);
	const uint2 input_pixel = uint2(uv * input_resolution);
	uint2 dim;
	uint sampleCount;
	input_stencil.GetDimensions(dim.x, dim.y, sampleCount);
	coverage = 0;
	for(uint sam = 0; sam < sampleCount; ++sam)
	{
		if ((input_stencil.Load(input_pixel, sam) & push.bit) != 0)
			coverage |= 1u << sam;
	}
}

#else

Texture2D<uint> input_stencil : register(t0);

void main(float4 pos : SV_Position)
{
	const float2 uv = pos.xy * push.output_resolution_rcp;
	const uint2 input_resolution = uint2(push.input_resolution & 0xFFFF, push.input_resolution >> 16u);
	const uint2 input_pixel = uint2(uv * input_resolution);
	if ((input_stencil[input_pixel] & push.bit) == 0)
		discard;
}

#endif // MSAA
