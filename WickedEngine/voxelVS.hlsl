#include "globals.hlsli"
#include "cube.hlsli"
#include "voxelHF.hlsli"

struct VSOut
{
	float4 pos : SV_POSITION;
	float4 col : TEXCOORD;
};

VSOut main( uint vertexID : SV_VERTEXID )
{
	VSOut o;

	uint3 coord = unflatten3D(vertexID, g_xFrame_VoxelRadianceDataRes);
	o.pos = float4(coord, 1);
	o.col = texture_voxelradiance[coord];

	//[branch]
	//if (o.col.a > 0)
	//{
	//	float3 pos = (float3)coord / g_xFrame_VoxelRadianceDataRes * 2 - 1;
	//	pos.y = -pos.y;
	//	pos *= g_xFrame_VoxelRadianceDataRes;
	//	pos += /*CUBE[vertexID].xyz*/CreateCube(vertexID) * 2 /*+ float3(1,-1,1)*/;
	//	pos *= g_xFrame_VoxelRadianceDataRes * g_xFrame_VoxelRadianceDataSize / g_xFrame_VoxelRadianceDataRes;

	//	o.pos = mul(float4(pos, 1), g_xTransform);
	//	o.col *= g_xColor;
	//}
	//else
	//{
	//	o.pos = 0;
	//}

	return o;
}