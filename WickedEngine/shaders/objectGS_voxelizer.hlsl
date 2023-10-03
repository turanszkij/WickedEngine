#include "globals.hlsli"
#include "objectHF.hlsli"

struct GSInput
{
	float4 pos : SV_POSITION;
	float4 uvsets : UVSETS;
	min16float4 color : COLOR;
	min16float3 nor : NORMAL;
};

// Note: centroid interpolation is used to avoid floating voxels in some cases
struct GSOutput
{
	float4 pos : SV_POSITION;
	centroid float4 uvsets : UVSETS;
	centroid min16float4 color : COLOR;
	centroid min16float3 N : NORMAL;
	centroid float3 P : POSITION3D;

#ifdef VOXELIZATION_CONSERVATIVE_RASTERIZATION_ENABLED
	nointerpolation float3 aabb_min : AABB_MIN;
	nointerpolation float3 aabb_max : AABB_MAX;
#endif // VOXELIZATION_CONSERVATIVE_RASTERIZATION_ENABLED
};

[maxvertexcount(3)]
void main(
	triangle GSInput input[3],
	inout TriangleStream< GSOutput > outputStream
)
{
	VoxelClipMap clipmap = GetFrame().vxgi.clipmaps[g_xVoxelizer.clipmap_index];

	float3 facenormal = abs(input[0].nor + input[1].nor + input[2].nor);
	uint maxi = facenormal[1] > facenormal[0] ? 1 : 0;
	maxi = facenormal[2] > facenormal[maxi] ? 2 : maxi;

	float3 aabb_min = min(input[0].pos.xyz, min(input[1].pos.xyz, input[2].pos.xyz));
	float3 aabb_max = max(input[0].pos.xyz, max(input[1].pos.xyz, input[2].pos.xyz));

	GSOutput output[3];

	uint i = 0;
	for (i = 0; i < 3; ++i)
	{
		// World space -> Voxel grid space:
		output[i].pos.xyz = (input[i].pos.xyz - clipmap.center) / clipmap.voxelSize;

		// Project onto dominant axis:
		[flatten]
		if (maxi == 0)
		{
			output[i].pos.xyz = output[i].pos.zyx;
		}
		else if (maxi == 1)
		{
			output[i].pos.xyz = output[i].pos.xzy;
		}
	}

#ifdef VOXELIZATION_CONSERVATIVE_RASTERIZATION_ENABLED
	// Expand triangle to get fake Conservative Rasterization:
	float2 side0N = normalize(output[1].pos.xy - output[0].pos.xy);
	float2 side1N = normalize(output[2].pos.xy - output[1].pos.xy);
	float2 side2N = normalize(output[0].pos.xy - output[2].pos.xy);
	output[0].pos.xy += normalize(side2N - side0N);
	output[1].pos.xy += normalize(side0N - side1N);
	output[2].pos.xy += normalize(side1N - side2N);
#endif // VOXELIZATION_CONSERVATIVE_RASTERIZATION_ENABLED

	for (i = 0; i < 3; ++i)
	{
		// Voxel grid space -> Clip space
		output[i].pos.xy *= GetFrame().vxgi.resolution_rcp;
		output[i].pos.zw = 1;

		// Append the rest of the parameters as is:
		output[i].color = input[i].color;
		output[i].uvsets = input[i].uvsets;
		output[i].N = input[i].nor;
		output[i].P = input[i].pos.xyz;

#ifdef VOXELIZATION_CONSERVATIVE_RASTERIZATION_ENABLED
		output[i].aabb_min = aabb_min;
		output[i].aabb_max = aabb_max;
#endif // VOXELIZATION_CONSERVATIVE_RASTERIZATION_ENABLED

		outputStream.Append(output[i]);
	}
}
