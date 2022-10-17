#include "globals.hlsli"
#include "ShaderInterop_Renderer.h"

PUSHCONSTANT(push, VirtualTextureTileRequestsPush);

groupshared uint lod_offsets[9];

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex)
{
	ByteAddressBuffer lodOffsetsBuffer = bindless_buffers[push.lodOffsetsBufferRO];
	Texture2D<uint> feedbackTexture = bindless_textures_uint[push.feedbackTextureRO];
	RWByteAddressBuffer requestBuffer = bindless_rwbuffers[push.requestBufferRW];

	if (groupIndex < 9)
	{
		lod_offsets[groupIndex] = lodOffsetsBuffer.Load(groupIndex * sizeof(uint));
	}

	GroupMemoryBarrierWithGroupSync();

	if (DTid.x >= push.width / 2 || DTid.y >= push.height / 2)
		return;

	const uint4 page_requests = uint4(
		feedbackTexture[DTid.xy * 2 + uint2(0, 0)],
		feedbackTexture[DTid.xy * 2 + uint2(1, 0)],
		feedbackTexture[DTid.xy * 2 + uint2(0, 1)],
		feedbackTexture[DTid.xy * 2 + uint2(1, 1)]
	);
	const uint page_request = min(page_requests.x, min(page_requests.y, min(page_requests.z, page_requests.w)));

	const uint x = DTid.x * 2;
	const uint y = DTid.y * 2;

	// LOD0 :
	//	- Extrude min request area to 2x2 tiles
	//	- No need for atomics here
	requestBuffer.Store(((x + 0) + (y + 0) * push.width) * sizeof(uint), page_request);
	requestBuffer.Store(((x + 1) + (y + 0) * push.width) * sizeof(uint), page_request);
	requestBuffer.Store(((x + 0) + (y + 1) * push.width) * sizeof(uint), page_request);
	requestBuffer.Store(((x + 1) + (y + 1) * push.width) * sizeof(uint), page_request);

	// LOD1 :
	//	- No need for atomics here
	requestBuffer.Store((lod_offsets[1] + DTid.x + DTid.y * (push.width >> 1)) * sizeof(uint), page_request);

	// LOD2 -> LOD MAX:
	//	- Need atomics
	for (uint lod = 2; lod < push.lodCount; ++lod)
	{
		const uint l_x = x >> lod;
		const uint l_y = y >> lod;
		const uint l_width = max(1u, push.width >> lod);
		const uint l_page_offset = lod_offsets[lod];
		const uint l_index = l_page_offset + l_x + l_y * l_width;
		requestBuffer.InterlockedMin(l_index * sizeof(uint), page_request);
	}
}
