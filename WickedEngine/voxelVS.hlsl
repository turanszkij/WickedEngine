#include "globals.hlsli"
#include "cube.hlsli"
#include "voxelHF.hlsli"

struct VSOut
{
	float4 pos : SV_POSITION;
	float4 col : TEXCOORD;
};

VSOut main( uint vertexID : SV_VERTEXID, uint instanceID : SV_INSTANCEID )
{
	VSOut o;

	uint3 dim;
	texture_voxelradiance.GetDimensions(dim.x, dim.y, dim.z);

	uint3 coord = to3D(instanceID, dim);
	o.col = texture_voxelradiance[coord];


	[branch]
	if (o.col.a > 0)
	{
		float3 pos = (float3)coord / (float3)dim * 2 - 1;
		pos.y = -pos.y;
		pos.xyz *= dim;
		pos += CUBE[vertexID].xyz;
		pos *= dim * g_xWorld_VoxelRadianceRemap;

		o.pos = mul(float4(pos, 1), g_xTransform);
		o.col *= g_xColor;
	}
	else
	{
		o.pos = 0;
	}

	return o;
}