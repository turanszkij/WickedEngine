#include "globals.hlsli"
#include "imageHF.hlsli"

static const float2 QUAD_EDGE[] = {
	float2(-1, 1),
	float2(1, 1),
	float2(-1, -1),
	float2(1, -1),
};

VertextoPixel main(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
{
	VertextoPixel Out = (VertextoPixel)0;

	[branch]
	if (image.IsFont())
	{
		// Font renderer:
		uint vID = instanceID * 4 + vertexID;
		FontVertex vertex = bindless_buffers[descriptor_index(image.buffer_index)].Load<FontVertex>(image.buffer_offset + vID * sizeof(FontVertex));

		Out.pos = mul(image.transform, float4(asfloat(vertex.pos), 0, 1));
		Out.q = vertex.uv;
		switch (vertexID)
		{
			default:
			case 0:
				Out.edge = float2(0, 0);
				break;
			case 1:
				Out.edge = float2(1, 0);
				break;
			case 2:
				Out.edge = float2(0, 1);
				break;
			case 3:
				Out.edge = float2(1, 1);
				break;
		}
	}
	else
	{
		// Image renderer:
		[branch]
		if (image.flags & IMAGE_FLAG_FULLSCREEN)
		{
			vertexID_create_fullscreen_triangle(vertexID, Out.pos);
		}
		else
		{
			Out.pos = bindless_buffers[descriptor_index(image.buffer_index)].Load<float4>(image.buffer_offset + vertexID * sizeof(float4));

			// Set up inverse bilinear interpolation
			Out.q = Out.pos.xy - image.b0;

			if (image.flags & IMAGE_FLAG_CORNER_ROUNDING)
			{
				// triangle fan, complex shape; center vertex is not edge, rest are edge:
				Out.edge = (vertexID & 1) ? 0 : 1;
			}
			else
			{
				// simple rectange shape, edge weight is based on uvs:
				Out.edge = QUAD_EDGE[vertexID];
			}
		}
		Out.screen = Out.pos;
	}
	return Out;
}

