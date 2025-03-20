#include "globals.hlsli"
// This shader is running 8 full screen passes for each stencil bit to extract a color image
//	Note: The simpler method is to use CopyTexture from ImageAspect::COLOR to ImageAspect::STENCIL
//	But Vulkan doesn't support that currently, so this is a workaround for that
//	Vulkan issue: https://github.com/KhronosGroup/Vulkan-Docs/issues/2079

PUSHCONSTANT(push, StencilBitPush);

uint main() : SV_TARGET
{
	return push.bit;
}
