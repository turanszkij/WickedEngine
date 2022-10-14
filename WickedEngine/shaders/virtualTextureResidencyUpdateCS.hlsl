#include "globals.hlsli"
#include "ShaderInterop_Renderer.h"

PUSHCONSTANT(push, VirtualTextureResidencyUpdatePush);

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
	if (DTid.x >= push.width || DTid.y >= push.height)
		return;
	ByteAddressBuffer lodOffsetsBuffer = bindless_buffers[push.lodOffsetsBufferRO];
	ByteAddressBuffer pageBuffer = bindless_buffers[push.pageBufferRO];
	RWTexture2D<uint> residencyTexture = bindless_rwtextures_uint[push.residencyTextureRW];

	residencyTexture[DTid.xy] = push.lodCount;

	const uint x = DTid.x;
	const uint y = DTid.y;
	for (uint lod = 0; lod < push.lodCount; ++lod)
	{
		const uint l_x = x >> lod;
		const uint l_y = y >> lod;
		const uint l_width = max(1u, push.width >> lod);
		const uint l_page_offset = lodOffsetsBuffer.Load(lod * sizeof(uint));
		const uint l_index = l_page_offset + l_x + l_y * l_width;
		const uint page = pageBuffer.Load(l_index * sizeof(uint));
		if (page != 0xFFFF)
		{
			InterlockedMin(residencyTexture[DTid.xy], lod);
		}
	}
}
