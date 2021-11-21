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

	uint3 coord = unflatten3D(vertexID, GetFrame().voxelradiance_resolution);
	o.pos = float4(coord, 1);
	o.col = texture_voxelgi[coord];

	return o;
}
