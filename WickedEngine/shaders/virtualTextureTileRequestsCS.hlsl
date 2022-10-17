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

	if (DTid.x >= push.width || DTid.y >= push.height)
		return;

	uint page_request = feedbackTexture[DTid.xy];

	const uint x = DTid.x;
	const uint y = DTid.y;
	for (uint lod = 0; lod < push.lodCount; ++lod)
	{
		const uint l_x = x >> lod;
		const uint l_y = y >> lod;
		const uint l_width = max(1u, push.width >> lod);
		const uint l_page_offset = lod_offsets[lod];
		const uint l_index = l_page_offset + l_x + l_y * l_width;
		requestBuffer.InterlockedMin(l_index * sizeof(uint), page_request);
	}
}
