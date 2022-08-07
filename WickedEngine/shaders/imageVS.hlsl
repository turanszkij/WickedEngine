#include "globals.hlsli"
#include "imageHF.hlsli"

VertextoPixel main(uint vI : SV_VertexID)
{
	VertextoPixel Out;

	[branch]
	if (image.flags & IMAGE_FLAG_FULLSCREEN)
	{
		vertexID_create_fullscreen_triangle(vI, Out.pos);
	}
	else
	{
		Out.pos = bindless_buffers[image.buffer_index].Load<float4>(image.buffer_offset + vI * sizeof(float4));

		// Set up inverse bilinear interpolation
		Out.q = Out.pos.xy - unpack_half2(image.b0);
	}

	Out.screen = Out.pos;
	return Out;
}

