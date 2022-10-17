#include "globals.hlsli"
#include "ShaderInterop_Renderer.h"

PUSHCONSTANT(push, VirtualTextureTileAllocatePush);

groupshared uint lod_offsets[9];

[numthreads(64, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex)
{
	ByteAddressBuffer lodOffsetsBuffer = bindless_buffers[push.lodOffsetsBufferRO];
	ByteAddressBuffer pageBuffer = bindless_buffers[push.pageBufferRO];
	RWByteAddressBuffer requestBuffer = bindless_rwbuffers[push.requestBufferRW];
	RWByteAddressBuffer allocationBuffer = bindless_rwbuffers[push.allocationBufferRW];

	if (groupIndex < 9)
	{
		lod_offsets[groupIndex] = lodOffsetsBuffer.Load(groupIndex * sizeof(uint));
	}

	GroupMemoryBarrierWithGroupSync();

	if (DTid.x >= push.threadCount)
		return;

	uint lod = 1;
	for (; lod < push.lodCount; ++lod) // this should be small loop to look up which lod we are processing
	{
		if (lod_offsets[lod] > DTid.x)
			break;
	}
	lod--;

	if (lod >= 9)
		return;

	const uint page = pageBuffer.Load(DTid.x * sizeof(uint));
	const uint requestMinLod = requestBuffer.Load(DTid.x * sizeof(uint));
	const bool must_be_always_resident = lod == ((int)push.lodCount - 2); // bottom 2 lods (both only 1 tile each) always resident

	const uint l_width = max(1u, push.width >> lod);
	const uint l_index = DTid.x - lod_offsets[lod];
	const uint x = l_index % l_width;
	const uint y = l_index / l_width;

	if (page == 0xFFFF && (requestMinLod <= lod || must_be_always_resident))
	{
#if 0 // if enabled, it will slow down allocation and only allocate 1 level per frame
		if (lod < push.lodCount - 1)
		{
			// Check if LOD below this is resident, otherwise don't allocate it yet
			const uint ll_x = x >> 1;
			const uint ll_y = y >> 1;
			const uint lower_lod = lod + 1;
			const uint ll_width = max(1u, push.width >> lower_lod);
			const uint ll_offset = lod_offsets[lower_lod];
			const uint ll_index = ll_offset + flatten2D(uint2(ll_x, ll_y), ll_width);
			if (pageBuffer.Load(ll_index * sizeof(uint)) == 0xFFFF)
				return;
		}
#endif
		// Allocate:
		uint allocationIndex = 0;
		allocationBuffer.InterlockedAdd(0, 1, allocationIndex);
		allocationBuffer.Store((1 + allocationIndex) * sizeof(uint), ((x & 0xFF) << 24u) | ((y & 0xFF) << 16u) | ((lod & 0xFF) << 8u) | 1u);
	}
	else if(page != 0xFFFF && requestMinLod > lod && !must_be_always_resident)
	{
		if (lod > 0)
		{
			// Check if LOD above is fully non-resident, otherwise we mustn't deallocate this
			const uint ll_x = x << 1;
			const uint ll_y = y << 1;
			const uint upper_lod = lod - 1;
			const uint ll_width = max(1u, push.width >> upper_lod);
			const uint ll_offset = lod_offsets[upper_lod];
			const uint ll_index_0 = ll_offset + flatten2D(uint2(ll_x + 0, ll_y + 0), ll_width);
			const uint ll_index_1 = ll_offset + flatten2D(uint2(ll_x + 1, ll_y + 0), ll_width);
			const uint ll_index_2 = ll_offset + flatten2D(uint2(ll_x + 0, ll_y + 1), ll_width);
			const uint ll_index_3 = ll_offset + flatten2D(uint2(ll_x + 1, ll_y + 1), ll_width);
			if (pageBuffer.Load(ll_index_0 * sizeof(uint)) != 0xFFFF)
				return;
			if (pageBuffer.Load(ll_index_1 * sizeof(uint)) != 0xFFFF)
				return;
			if (pageBuffer.Load(ll_index_2 * sizeof(uint)) != 0xFFFF)
				return;
			if (pageBuffer.Load(ll_index_3 * sizeof(uint)) != 0xFFFF)
				return;
		}
		// Deallocate:
		uint allocationIndex = 0;
		allocationBuffer.InterlockedAdd(0, 1, allocationIndex);
		allocationBuffer.Store((1 + allocationIndex) * sizeof(uint), ((x & 0xFF) << 24u) | ((y & 0xFF) << 16u) | ((lod & 0xFF) << 8u) | 0u);
	}
}
