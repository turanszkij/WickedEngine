#include "globals.hlsli"
#include "ShaderInterop_Renderer.h"

PUSHCONSTANT(push, VirtualTextureTileAllocatePush);

[numthreads(64, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex)
{
	if (DTid.x >= push.threadCount)
		return;

	Buffer<uint> pageBuffer = bindless_buffers_uint[push.pageBufferRO];
	RWByteAddressBuffer requestBuffer = bindless_rwbuffers[push.requestBufferRW];
	RWByteAddressBuffer allocationBuffer = bindless_rwbuffers[push.allocationBufferRW];

	uint page_count = 0;
	uint lod_offsets[10];
	for (uint i = 0; i < push.lodCount; ++i)
	{
		const uint l_width = max(1u, push.width >> i);
		const uint l_height = max(1u, push.height >> i);
		lod_offsets[i] = page_count;
		page_count += l_width * l_height;
	}

	uint lod = 1;
	for (; lod < push.lodCount; ++lod) // this should be small loop to look up which lod we are processing
	{
		if (lod_offsets[lod] > DTid.x)
			break;
	}
	lod--;

	if (lod >= 9)
		return;

	const uint page = pageBuffer[DTid.x];
	const uint requestMinLod = requestBuffer.Load(DTid.x * sizeof(uint));

	const uint l_width = max(1u, push.width >> lod);
	const uint l_index = DTid.x - lod_offsets[lod];
	const uint x = l_index % l_width;
	const uint y = l_index / l_width;

	if (requestMinLod <= lod)
	{
		// Allocation request:
		uint allocationIndex = 0;
		allocationBuffer.InterlockedAdd(0, 1, allocationIndex);
		allocationBuffer.Store((1 + allocationIndex) * sizeof(uint), ((x & 0xFF) << 24u) | ((y & 0xFF) << 16u) | ((lod & 0xFF) << 8u));
	}
}
