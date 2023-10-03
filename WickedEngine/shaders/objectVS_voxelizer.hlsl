#define OBJECTSHADER_LAYOUT_COMMON
#define DISABLE_WIND
#include "objectHF.hlsli"

struct VSOut
{
	float4 pos : SV_POSITION;
	float4 uvsets : UVSETS;
	min16float4 color : COLOR;
	min16float3 N : NORMAL;
#ifndef VOXELIZATION_GEOMETRY_SHADER_ENABLED
	float3 P : POSITION3D;
#endif // VOXELIZATION_GEOMETRY_SHADER_ENABLED
};

VSOut main(VertexInput input)
{
	VertexSurface surface;
	surface.create(GetMaterial(), input);

	VSOut Out;
	Out.pos = surface.position;
	Out.color = surface.color;
	Out.uvsets = surface.uvsets;
	Out.N = surface.normal;

#ifndef VOXELIZATION_GEOMETRY_SHADER_ENABLED
	Out.P = surface.position.xyz;

	VoxelClipMap clipmap = GetFrame().vxgi.clipmaps[g_xVoxelizer.clipmap_index];

	// World space -> Voxel grid space:
	Out.pos.xyz = (Out.pos.xyz - clipmap.center) / clipmap.voxelSize;

	// Project onto dominant axis:
	const uint frustum_index = input.GetInstancePointer().GetCameraIndex();
	switch (frustum_index)
	{
	default:
	case 0:
		Out.pos.xyz = Out.pos.xyz;
		break;
	case 1:
		Out.pos.xyz = Out.pos.zyx;
		break;
	case 2:
		Out.pos.xyz = Out.pos.xzy;
		break;

	// Test: if rendered with 6 frustums, double sided voxelization happens here:
	case 3:
		Out.pos.xyz = Out.pos.xyz * float3(1, 1, -1);
		Out.N *= -1;
		break;
	case 4:
		Out.pos.xyz = Out.pos.zyx * float3(1, 1, -1);
		Out.N *= -1;
		break;
	case 5:
		Out.pos.xyz = Out.pos.xzy * float3(1, 1, -1);
		Out.N *= -1;
		break;
	}

	// Voxel grid space -> Clip space
	Out.pos.xy *= GetFrame().vxgi.resolution_rcp;
	Out.pos.zw = 1;
#endif // VOXELIZATION_GEOMETRY_SHADER_ENABLED

	return Out;
}

