#include "globals.hlsli"
#include "ShaderInterop_Renderer.h"

ConstantBuffer<VirtualTextureResidencyUpdateCB> cb : register(b0);

[numthreads(8, 8, 1)]
void main(uint3 DTid : SV_DispatchThreadID, uint groupIndex : SV_GroupIndex)
{
	if (DTid.x >= cb.width || DTid.y >= cb.height)
		return;

	ByteAddressBuffer pageBuffer = bindless_buffers[cb.pageBufferRO];

	uint minLod = 0xFF;
	uint tile_x = 0;
	uint tile_y = 0;

	const uint x = DTid.x;
	const uint y = DTid.y;
	uint lod_offset = 0;
	uint lod_pages[10];
	uint lod = 0;
	for (lod = 0; lod < cb.lodCount; ++lod)
	{
		const uint l_x = x >> lod;
		const uint l_y = y >> lod;
		const uint l_width = max(1u, cb.width >> lod);
		const uint l_height = max(1u, cb.height >> lod);
		const uint l_page_offset = lod_offset;
		const uint l_index = l_page_offset + l_x + l_y * l_width;
		const uint page = pageBuffer.Load(l_index * sizeof(uint));
		lod_pages[lod] = page;
		if (page == 0xFFFF)
		{
			// invalid page
			//	normally this wouldn't happen after a resident mip was encountered, but
			//	it can happen on allocation failures, in this case reset the residency to invalid
			//	this will mean that while there was a higher res resident page, but we can't use that because lower levels might be non resident
			minLod = 0xFF;
		}
		else if(lod < minLod)
		{
			// valid page
			//	normally this would be the highest lod and we could exit, but failed allocations can cause holes in the mip chain
			minLod = lod;
			tile_x = page & 0xFF;
			tile_y = (page >> 8u) & 0xFF;
		}
		lod_offset += l_width * l_height;
	}

	// Update the mip chain:
	for (lod = 0; lod < cb.lodCount; ++lod)
	{
		uint lod_check = 1u << lod;

		if ((DTid.x % lod_check == 0) && (DTid.y % lod_check == 0))
		{
			uint page = lod_pages[lod];
			uint2 write_coord = DTid.xy >> lod;

			const bool packed_mips = lod == cb.lodCount - 1;
			if (packed_mips)
			{
				RWTexture2D<uint> residencyTexture = bindless_rwtextures_uint[cb.residencyTextureRW_mips[lod - 1].x];
				residencyTexture[write_coord] &= 0xFFFF;
				uint x = page & 0xFF;
				uint y = (page >> 8u) & 0xFF;
				residencyTexture[write_coord] |= ((x & 0xFF) << 16u) | ((y & 0xFF) << 24u);
				//residencyTexture[write_coord] |= 0xFFFF << 24u;
			}
			else
			{
				RWTexture2D<uint> residencyTexture = bindless_rwtextures_uint[cb.residencyTextureRW_mips[lod].x];
				if (lod < minLod || page == 0xFFFF)
				{
					residencyTexture[write_coord] = (tile_x & 0xFF) | ((tile_y & 0xFF) << 8u) | ((minLod & 0xFF) << 16u);
				}
				else
				{
					uint x = page & 0xFF;
					uint y = (page >> 8u) & 0xFF;
					residencyTexture[write_coord] = (x & 0xFF) | ((y & 0xFF) << 8u) | ((lod & 0xFF) << 16u);
				}
			}
		}
	}
}
