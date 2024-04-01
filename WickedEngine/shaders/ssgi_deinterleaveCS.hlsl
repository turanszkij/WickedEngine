#include "globals.hlsli"
#include "ShaderInterop_Postprocess.h"

RWTexture2DArray<float>		atlas2x_depth : register(u0);
RWTexture2DArray<float>		atlas4x_depth : register(u1);
RWTexture2DArray<float>		atlas8x_depth : register(u2);
RWTexture2DArray<float>		atlas16x_depth : register(u3);
RWTexture2D<float>			regular2x_depth : register(u4);
RWTexture2D<float2>			regular2x_normal : register(u5);
RWTexture2D<float>			regular4x_depth : register(u6);
RWTexture2D<float2>			regular4x_normal : register(u7);
RWTexture2D<float>			regular8x_depth : register(u8);
RWTexture2D<float2>			regular8x_normal : register(u9);
RWTexture2D<float>			regular16x_depth : register(u10);
RWTexture2D<float2>			regular16x_normal : register(u11);

groupshared float shared_depths[256];
groupshared float2 shared_normals[256];

[numthreads(8, 8, 1)]
void main(uint3 Gid : SV_GroupID, uint groupIndex : SV_GroupIndex, uint3 GTid : SV_GroupThreadID, uint3 DTid : SV_DispatchThreadID)
{
	uint2 dim;
	texture_depth.GetDimensions(dim.x, dim.y);

    uint2 startST = Gid.xy << 4 | GTid.xy;
    uint destIdx = GTid.y << 4 | GTid.x;
    shared_depths[destIdx + 0] = texture_depth[min(startST | uint2(0, 0), dim - 1)];
    shared_depths[destIdx + 8] = texture_depth[min(startST | uint2(8, 0), dim - 1)];
    shared_depths[destIdx + 128] = texture_depth[min(startST | uint2(0, 8), dim - 1)];
    shared_depths[destIdx + 136] = texture_depth[min(startST | uint2(8, 8), dim - 1)];

    shared_normals[destIdx + 0] = texture_normal[min(startST | uint2(0, 0), dim - 1)];
    shared_normals[destIdx + 8] = texture_normal[min(startST | uint2(8, 0), dim - 1)];
    shared_normals[destIdx + 128] = texture_normal[min(startST | uint2(0, 8), dim - 1)];
    shared_normals[destIdx + 136] = texture_normal[min(startST | uint2(8, 8), dim - 1)];

    GroupMemoryBarrierWithGroupSync();

    uint ldsIndex = (GTid.x << 1) | (GTid.y << 5);

    float depth = shared_depths[ldsIndex];
    float2 normal = shared_normals[ldsIndex];

    uint2 st = DTid.xy;
    uint slice = flatten2D(st % 4, 4);
    atlas2x_depth[uint3(st >> 2, slice)] = depth;
    regular2x_depth[st] = depth;
    regular2x_normal[st] = normal;

    if (all(GTid.xy % 2) == 0)
    {
        st = DTid.xy >> 1;
        slice = flatten2D(st % 4, 4);
        atlas4x_depth[uint3(st >> 2, slice)] = depth;
		regular4x_depth[st] = depth;
		regular4x_normal[st] = normal;
		
		if (all(GTid.xy % 4) == 0)
		{
			st = DTid.xy >> 2;
			slice = flatten2D(st % 4, 4);
			atlas8x_depth[uint3(st >> 2, slice)] = depth;
			regular8x_depth[st] = depth;
			regular8x_normal[st] = normal;
			
			if (groupIndex == 0)
			{
				st = DTid.xy >> 3;
				slice = flatten2D(st % 4, 4);
				atlas16x_depth[uint3(st >> 2, slice)] = depth;
				regular16x_depth[st] = depth;
				regular16x_normal[st] = normal;
			}
		}
    }

}
