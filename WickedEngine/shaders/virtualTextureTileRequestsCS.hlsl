#include "globals.hlsli"
#include "ShaderInterop_Renderer.h"

PUSHCONSTANT(push, VirtualTextureTileRequestsPush);

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex)
{
	if (DTid.x >= push.width / 2 || DTid.y >= push.height / 2)
		return;

	Texture2D<uint> feedbackTexture = bindless_textures_uint[push.feedbackTextureRO];
	RWByteAddressBuffer requestBuffer = bindless_rwbuffers[push.requestBufferRW];

	uint page_count = 0;
	uint lod_offsets[10];
	for (uint i = 0; i < push.lodCount; ++i)
	{
		const uint l_width = max(1u, push.width >> i);
		const uint l_height = max(1u, push.height >> i);
		lod_offsets[i] = page_count;
		page_count += l_width * l_height;
	}

	const uint4 page_requests = uint4(
		feedbackTexture[DTid.xy * 2 + uint2(0, 0)],
		feedbackTexture[DTid.xy * 2 + uint2(1, 0)],
		feedbackTexture[DTid.xy * 2 + uint2(0, 1)],
		feedbackTexture[DTid.xy * 2 + uint2(1, 1)]
	);

	const uint x = DTid.x * 2;
	const uint y = DTid.y * 2;

	// LOD0 :
	//	- No need for atomics here
	requestBuffer.Store(((x + 0) + (y + 0) * push.width) * sizeof(uint), page_requests.x);
	requestBuffer.Store(((x + 1) + (y + 0) * push.width) * sizeof(uint), page_requests.y);
	requestBuffer.Store(((x + 0) + (y + 1) * push.width) * sizeof(uint), page_requests.z);
	requestBuffer.Store(((x + 1) + (y + 1) * push.width) * sizeof(uint), page_requests.w);

	const uint page_request = min(page_requests.x, min(page_requests.y, min(page_requests.z, page_requests.w)));

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
		requestBuffer.InterlockedMin(uint(l_index * sizeof(uint)), page_request);
	}
}
