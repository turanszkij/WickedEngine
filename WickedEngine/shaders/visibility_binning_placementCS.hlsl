#include "globals.hlsli"
#include "ShaderInterop_Renderer.h"
#include "brdf.hlsli"

RWStructuredBuffer<ShaderTypeBin> output_binning : register(u14);
RWStructuredBuffer<uint> output_binned_pixels : register(u15);

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	uint2 pixel = DTid.xy;

	uint primitiveID = texture_primitiveID[pixel];
	if (any(primitiveID))
	{
		PrimitiveID prim;
		prim.unpack(primitiveID);

		Surface surface;
		surface.init();

		if (surface.preload_internal(prim))
		{
			uint placement = 0;
			InterlockedAdd(output_binning[surface.material.shaderType].count, 1, placement);
			placement += output_binning[surface.material.shaderType].offset;
			output_binned_pixels[placement] = pack_pixel(pixel);
		}
	}
}
