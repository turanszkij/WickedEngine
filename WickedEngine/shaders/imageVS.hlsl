#include "globals.hlsli"
#include "imageHF.hlsli"

static const float2 QUAD_EDGE[] = {
	float2(-1, 1),
	float2(1, 1),
	float2(-1, -1),
	float2(1, -1),
};

VertextoPixel main(uint vI : SV_VertexID)
{
	VertextoPixel Out;
	Out.edge = 0;

	[branch]
	if (image.flags & IMAGE_FLAG_FULLSCREEN)
	{
		vertexID_create_fullscreen_triangle(vI, Out.pos);
	}
	else
	{
		Out.pos = bindless_buffers[image.buffer_index].Load<float4>(image.buffer_offset + vI * sizeof(float4));

		// Set up inverse bilinear interpolation
		Out.q = Out.pos.xy - image.b0;

		if (image.flags & IMAGE_FLAG_CORNER_ROUNDING)
		{
			// triangle fan, complex shape; center vertex is not edge, rest are edge:
			Out.edge = vI == 0 ? 0 : 1;
		}
		else
		{
			// simple rectange shape, edge weight is based on uvs:
			Out.edge = QUAD_EDGE[vI];
		}
	}

	Out.screen = Out.pos;
	return Out;
}

