#include "globals.hlsli"
// This shader is writing stencil with DepthStencilState and only discarding pixels
//	if bit is not contained in the stencilMask texture
//	With 8 full screen passes, it is possible to copy over all bits from stencilMask
//	texture into a real depth stencil buffer
//
//	Note: The simpler method is to use CopyTexture from ImageAspect::COLOR to ImageAspect::STENCIL
//	But Vulkan doesn't support that currently, so this is a workaround for that
//	Vulkan issue: https://github.com/KhronosGroup/Vulkan-Docs/issues/2079

Texture2D<uint> input_stencil : register(t0);

struct StencilBitPush
{
	uint bit;
};
PUSHCONSTANT(push, StencilBitPush);

void main(float4 pos : SV_Position)
{
	if ((input_stencil[uint2(pos.xy)] & push.bit) == 0)
		discard;
}
