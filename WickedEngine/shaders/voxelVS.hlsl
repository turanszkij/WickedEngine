#include "globals.hlsli"
#include "cube.hlsli"
#include "voxelHF.hlsli"

uint main(uint vertexID : SV_VERTEXID) : VERTEXID
{
	return vertexID;
}
