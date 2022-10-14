#include "globals.hlsli"
#include "ShaderInterop_Renderer.h"

PUSHCONSTANT(push, VirtualTextureTileAllocatePush);

[numthreads(64, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	if (DTid.x >= push.threadCount)
		return;
	ByteAddressBuffer lodOffsetsBuffer = bindless_buffers[push.lodOffsetsBufferRO];
	ByteAddressBuffer pageBuffer = bindless_buffers[push.pageBufferRO];
	RWByteAddressBuffer requestBuffer = bindless_rwbuffers[push.requestBufferRW];
	RWByteAddressBuffer allocationBuffer = bindless_rwbuffers[push.allocationBufferRW];

	uint lod = 0;
	for (; lod < push.lodCount; ++lod) // this should be small loop to look up which lod we are processing
	{
		if (lodOffsetsBuffer.Load(lod * sizeof(uint)) > DTid.x)
			break;
	}
	lod--;

	const uint page = pageBuffer.Load(DTid.x * sizeof(uint));
	const uint requestMinLod = requestBuffer.Load(DTid.x * sizeof(uint));
	const bool must_be_always_resident = lod == ((int)push.lodCount - 1);

	if (page == 0xFFFF && (requestMinLod <= lod || must_be_always_resident))
	{
		// Allocate:
		uint allocationIndex = 0;
		allocationBuffer.InterlockedAdd(0, 1, allocationIndex);
		allocationBuffer.Store((1 + allocationIndex) * sizeof(uint), ((DTid.x & 0xFFFF) << 16u) | ((lod & 0xFF) << 8u) | 1u);
	}
	else if(page != 0xFFFF && requestMinLod > lod && !must_be_always_resident)
	{
		// Deallocate:
		uint allocationIndex = 0;
		allocationBuffer.InterlockedAdd(0, 1, allocationIndex);
		allocationBuffer.Store((1 + allocationIndex) * sizeof(uint), ((DTid.x & 0xFFFF) << 16u) | ((lod & 0xFF) << 8u) | 0u);
	}
}
