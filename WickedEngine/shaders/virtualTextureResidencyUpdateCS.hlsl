#include "globals.hlsli"
#include "ShaderInterop_Renderer.h"

PUSHCONSTANT(push, VirtualTextureResidencyUpdatePush);

groupshared uint lod_offsets[9];

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex)
{
	ByteAddressBuffer lodOffsetsBuffer = bindless_buffers[push.lodOffsetsBufferRO];
	ByteAddressBuffer pageBuffer = bindless_buffers[push.pageBufferRO];
	RWTexture2D<uint> residencyTexture = bindless_rwtextures_uint[push.residencyTextureRW];

	if (groupIndex < 9)
	{
		lod_offsets[groupIndex] = lodOffsetsBuffer.Load(groupIndex * sizeof(uint));
	}

	GroupMemoryBarrierWithGroupSync();

	if (DTid.x >= push.width || DTid.y >= push.height)
		return;

	uint residency = 0xFF;

	const uint x = DTid.x;
	const uint y = DTid.y;
	for (uint lod = 0; lod < push.lodCount; ++lod)
	{
		const uint l_x = x >> lod;
		const uint l_y = y >> lod;
		const uint l_width = max(1u, push.width >> lod);
		const uint l_page_offset = lod_offsets[lod];
		const uint l_index = l_page_offset + l_x + l_y * l_width;
		const uint page = pageBuffer.Load(l_index * sizeof(uint));
		if (page == 0xFFFF)
		{
			// invalid page
			//	normally this wouldn't happen after a resident mip was encountered, but
			//	it can happen on allocation failures, in this case reset the residency to invalid
			//	this will mean that while there was a higher res resident page, but we can't use that because lower levels might be non resident
			residency = 0xFF;
		}
		else
		{
			// valid page
			//	normally this would be the highest lod and we could exit, but failed allocations can cause holes in the mip chain
			residency = min(residency, lod);
		}
	}

	residencyTexture[DTid.xy] = residency;
}
