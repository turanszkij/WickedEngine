#include "wiTerrain.h"
#include "wiProfiler.h"
#include "wiTimer.h"
#include "wiRenderer.h"
#include "wiHelper.h"
#include "wiScene.h"
#include "wiPhysics.h"
#include "wiImage.h"
#include "wiFont.h"
#include "wiTextureHelper.h"
#include "wiBacklog.h"

#include <mutex>

using namespace wi::ecs;
using namespace wi::scene;
using namespace wi::graphics;

namespace wi::terrain
{
	struct ChunkIndices
	{
		wi::vector<uint32_t> indices;
		struct LOD
		{
			uint32_t indexOffset = 0;
			uint32_t indexCount = 0;
		};
		wi::vector<LOD> lods;

		ChunkIndices()
		{
			const int max_lod = (int)std::log2(chunk_width - 3) + 1;
			lods.resize(max_lod);
			for (int lod = 0; lod < max_lod; ++lod)
			{
				lods[lod].indexOffset = (uint32_t)indices.size();

				if (lod == 0)
				{
					for (int x = 0; x < chunk_width - 1; x++)
					{
						for (int z = 0; z < chunk_width - 1; z++)
						{
							int lowerLeft = x + z * chunk_width;
							int lowerRight = (x + 1) + z * chunk_width;
							int topLeft = x + (z + 1) * chunk_width;
							int topRight = (x + 1) + (z + 1) * chunk_width;

							indices.push_back(topLeft);
							indices.push_back(lowerLeft);
							indices.push_back(lowerRight);

							indices.push_back(topLeft);
							indices.push_back(lowerRight);
							indices.push_back(topRight);
						}
					}
				}
				else
				{
					const int step = 1 << lod;
					// inner grid:
					for (int x = 1; x < chunk_width - 2; x += step)
					{
						for (int z = 1; z < chunk_width - 2; z += step)
						{
							int lowerLeft = x + z * chunk_width;
							int lowerRight = (x + step) + z * chunk_width;
							int topLeft = x + (z + step) * chunk_width;
							int topRight = (x + step) + (z + step) * chunk_width;

							indices.push_back(topLeft);
							indices.push_back(lowerLeft);
							indices.push_back(lowerRight);

							indices.push_back(topLeft);
							indices.push_back(lowerRight);
							indices.push_back(topRight);
						}
					}
					// bottom border:
					for (int x = 0; x < chunk_width - 1; ++x)
					{
						const int z = 0;
						int current = x + z * chunk_width;
						int neighbor = x + 1 + z * chunk_width;
						int connection = 1 + ((x + (step + 1) / 2 - 1) / step) * step + (z + 1) * chunk_width;

						indices.push_back(current);
						indices.push_back(neighbor);
						indices.push_back(connection);

						if (((x - 1) % (step)) == step / 2) // halfway fill triangle
						{
							int connection1 = 1 + (((x - 1) + (step + 1) / 2 - 1) / step) * step + (z + 1) * chunk_width;

							indices.push_back(current);
							indices.push_back(connection);
							indices.push_back(connection1);
						}
					}
					// top border:
					for (int x = 0; x < chunk_width - 1; ++x)
					{
						const int z = chunk_width - 1;
						int current = x + z * chunk_width;
						int neighbor = x + 1 + z * chunk_width;
						int connection = 1 + ((x + (step + 1) / 2 - 1) / step) * step + (z - 1) * chunk_width;

						indices.push_back(current);
						indices.push_back(connection);
						indices.push_back(neighbor);

						if (((x - 1) % (step)) == step / 2) // halfway fill triangle
						{
							int connection1 = 1 + (((x - 1) + (step + 1) / 2 - 1) / step) * step + (z - 1) * chunk_width;

							indices.push_back(current);
							indices.push_back(connection1);
							indices.push_back(connection);
						}
					}
					// left border:
					for (int z = 0; z < chunk_width - 1; ++z)
					{
						const int x = 0;
						int current = x + z * chunk_width;
						int neighbor = x + (z + 1) * chunk_width;
						int connection = x + 1 + (((z + (step + 1) / 2 - 1) / step) * step + 1) * chunk_width;

						indices.push_back(current);
						indices.push_back(connection);
						indices.push_back(neighbor);

						if (((z - 1) % (step)) == step / 2) // halfway fill triangle
						{
							int connection1 = x + 1 + ((((z - 1) + (step + 1) / 2 - 1) / step) * step + 1) * chunk_width;

							indices.push_back(current);
							indices.push_back(connection1);
							indices.push_back(connection);
						}
					}
					// right border:
					for (int z = 0; z < chunk_width - 1; ++z)
					{
						const int x = chunk_width - 1;
						int current = x + z * chunk_width;
						int neighbor = x + (z + 1) * chunk_width;
						int connection = x - 1 + (((z + (step + 1) / 2 - 1) / step) * step + 1) * chunk_width;

						indices.push_back(current);
						indices.push_back(neighbor);
						indices.push_back(connection);

						if (((z - 1) % (step)) == step / 2) // halfway fill triangle
						{
							int connection1 = x - 1 + ((((z - 1) + (step + 1) / 2 - 1) / step) * step + 1) * chunk_width;

							indices.push_back(current);
							indices.push_back(connection);
							indices.push_back(connection1);
						}
					}
				}

				lods[lod].indexCount = (uint32_t)indices.size() - lods[lod].indexOffset;
			}
		}
	};
	static ChunkIndices chunk_indices;

	struct Generator
	{
		wi::scene::Scene scene; // The background generation thread can safely add things to this, it will be merged into the main scene when it is safe to do so
		wi::jobsystem::context workload;
		std::atomic_bool cancelled{ false };
	};

	void GPUPageAllocator::init(size_t _block_size, size_t _page_size)
	{
		block_size = _block_size;
		page_size = _page_size;
		blocks.clear();
		pages.clear();
		new_block();
	}
	void GPUPageAllocator::new_block()
	{
		GraphicsDevice* device = GetDevice();
		const uint16_t block_index = (uint16_t)blocks.size();
		GPUBuffer& block = blocks.emplace_back();
		GPUBufferDesc desc;
		desc.alignment = page_size;
		desc.size = AlignTo(block_size, page_size);
		desc.misc_flags = ResourceMiscFlag::SPARSE_TILE_POOL_TEXTURE_NON_RT_DS;
		bool success = device->CreateBuffer(&desc, nullptr, &block);
		assert(success);
		device->SetName(&block, "GPUPageAllocator::block");

		uint32_t page_count = uint32_t(desc.size / page_size);
		pages.reserve(pages.capacity() + page_count);
		for (uint32_t i = 0; i < page_count; ++i)
		{
			Page page;
			page.block = block_index;
			page.index = i;
			pages.push_back(page);
		}
	}
	static std::mutex locker;
	GPUPageAllocator::Page GPUPageAllocator::allocate()
	{
		std::scoped_lock lock(locker);
		while (pages.empty())
			new_block();
		Page page = pages.back();
		pages.pop_back();
		return page;
	}
	void GPUPageAllocator::free(Page page)
	{
		std::scoped_lock lock(locker);
		if (!page.IsValid())
			return;
		pages.push_back(page);
	}

	void VirtualTexture::init(GPUPageAllocator& page_allocator, const TextureDesc& desc)
	{
		GraphicsDevice* device = GetDevice();
		bool success = device->CreateTexture(&desc, nullptr, &texture);
		assert(success);

		for (uint32_t i = 0; i < texture.desc.mip_levels; ++i)
		{
			int subresource = 0;
			subresource = device->CreateSubresource(&texture, SubresourceType::UAV, 0, 1, i, 1);
			assert(subresource == i);
		}

		residencyMap = {};
		feedbackMap = {};
		requestBuffer = {};
		allocationBuffer = {};
		pageBuffer = {};

		uint32_t width = std::max(1u, texture.desc.width / texture.sparse_properties->tile_width);
		uint32_t height = std::max(1u, texture.desc.height / texture.sparse_properties->tile_height);
		lod_count = texture.desc.mip_levels;
		if (texture.sparse_properties->packed_mip_count > 0)
		{
			lod_count -= texture.sparse_properties->packed_mip_count - 1;
		}
		for (int i = 0; i < arraysize(data_available_CPU); ++i)
		{
			data_available_CPU[i] = false;
			allocationBuffer_CPU_readback[i] = {};
			pageBuffer_CPU_upload[i] = {};
		}
		cpu_resource_id = 0;
		pages.clear();

		uint32_t page_count = 0;
		for (uint32_t i = 0; i < lod_count; ++i)
		{
			const uint32_t l_width = std::max(1u, width >> i);
			const uint32_t l_height = std::max(1u, height >> i);
			page_count += l_width * l_height;
		}
		pages.resize(page_count);

		sparse_coordinate.clear();
		sparse_size.clear();
		tile_range_flags.clear();
		tile_range_offset.clear();
		tile_range_count.clear();
		last_block = 0;

		if (desc.width >= 1024)
		{
			TextureDesc td;
			td.width = width;
			td.height = height;

			td.format = Format::R8_UINT;
			td.bind_flags = BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
			td.usage = Usage::DEFAULT;
			td.layout = ResourceState::SHADER_RESOURCE_COMPUTE;
			success = device->CreateTexture(&td, nullptr, &residencyMap);
			assert(success);
			device->SetName(&residencyMap, "VirtualTexture::residencyMap");

#ifdef TERRAIN_VIRTUAL_TEXTURE_DEBUG
			td.misc_flags = {};
			td.bind_flags = BindFlag::NONE;
			td.usage = Usage::READBACK;
			for (int i = 0; i < arraysize(residencyMap_CPU_readback); ++i)
			{
				success = device->CreateTexture(&td, nullptr, &residencyMap_CPU_readback[i]);
				assert(success);
				device->SetName(&residencyMap_CPU_readback[i], "VirtualTexture::residencyMap_CPU_readback[i]");
			}
#endif // TERRAIN_VIRTUAL_TEXTURE_DEBUG

			td.format = Format::R32_UINT; // shader atomic support needed
			td.bind_flags = BindFlag::UNORDERED_ACCESS | BindFlag::SHADER_RESOURCE;
			td.usage = Usage::DEFAULT;
			td.layout = ResourceState::SHADER_RESOURCE_COMPUTE;
			success = device->CreateTexture(&td, nullptr, &feedbackMap);
			assert(success);
			device->SetName(&feedbackMap, "VirtualTexture::feedbackMap");

#ifdef TERRAIN_VIRTUAL_TEXTURE_DEBUG
			td.misc_flags = {};
			td.bind_flags = BindFlag::NONE;
			td.usage = Usage::READBACK;
			for (int i = 0; i < arraysize(feedbackMap_CPU_readback); ++i)
			{
				success = device->CreateTexture(&td, nullptr, &feedbackMap_CPU_readback[i]);
				assert(success);
				device->SetName(&feedbackMap_CPU_readback[i], "VirtualTexture::feedbackMap_CPU_readback[i]");
			}
#endif // TERRAIN_VIRTUAL_TEXTURE_DEBUG

			{
				GPUBufferDesc bd;
				bd.misc_flags = ResourceMiscFlag::BUFFER_RAW;
				bd.usage = Usage::DEFAULT;
				bd.bind_flags = BindFlag::UNORDERED_ACCESS;
				bd.size = sizeof(uint32_t) * page_count;
				success = device->CreateBuffer(&bd, nullptr, &requestBuffer);
				assert(success);
				device->SetName(&requestBuffer, "VirtualTexture::requestBuffer");

#ifdef TERRAIN_VIRTUAL_TEXTURE_DEBUG
				bd.misc_flags = {};
				bd.bind_flags = BindFlag::NONE;
				bd.usage = Usage::READBACK;
				for (int i = 0; i < arraysize(requestBuffer_CPU_readback); ++i)
				{
					success = device->CreateBuffer(&bd, nullptr, &requestBuffer_CPU_readback[i]);
					assert(success);
					device->SetName(&requestBuffer_CPU_readback[i], "VirtualTexture::requestBuffer_CPU_readback[i]");
				}
#endif // TERRAIN_VIRTUAL_TEXTURE_DEBUG
			}
			{
				GPUBufferDesc bd;
				bd.misc_flags = ResourceMiscFlag::BUFFER_RAW;
				bd.usage = Usage::DEFAULT;
				bd.bind_flags = BindFlag::UNORDERED_ACCESS;
				bd.size = sizeof(uint32_t) * (page_count + 1); // +1: atomic global counter
				success = device->CreateBuffer(&bd, nullptr, &allocationBuffer);
				assert(success);
				device->SetName(&allocationBuffer, "VirtualTexture::allocationBuffer");

				bd.misc_flags = {};
				bd.bind_flags = BindFlag::NONE;
				bd.usage = Usage::READBACK;
				for (int i = 0; i < arraysize(allocationBuffer_CPU_readback); ++i)
				{
					success = device->CreateBuffer(&bd, nullptr, &allocationBuffer_CPU_readback[i]);
					assert(success);
					device->SetName(&allocationBuffer_CPU_readback[i], "VirtualTexture::allocationBuffer_CPU_readback[i]");
				}
			}
			{
				GPUBufferDesc bd;
				bd.misc_flags = ResourceMiscFlag::BUFFER_RAW;
				bd.usage = Usage::DEFAULT;
				bd.bind_flags = BindFlag::SHADER_RESOURCE;
				bd.size = sizeof(uint32_t) * page_count;
				success = device->CreateBuffer(&bd, nullptr, &pageBuffer);
				assert(success);
				device->SetName(&pageBuffer, "VirtualTexture::pageBuffer");

				bd.misc_flags = {};
				bd.usage = Usage::UPLOAD;
				bd.bind_flags = {};
				for (int i = 0; i < arraysize(pageBuffer_CPU_upload); ++i)
				{
					success = device->CreateBuffer(&bd, nullptr, &pageBuffer_CPU_upload[i]);
					assert(success);
					device->SetName(&pageBuffer_CPU_upload[i], "VirtualTexture::pageBuffer_CPU_upload[i]");
				}
			}
		}

		locker.lock();
		if (page_allocator.blocks.empty())
		{
			page_allocator.init(64ull * 1024ull * 1024ull, texture.sparse_page_size);
		}
		locker.unlock();

		if (feedbackMap.IsValid())
		{
			// Allocate least detailed mip level up front:
			//	This is needed because for the first few frames the GPU readback won't be ready to use
			//	So in the first few frames after creation, the least detailed mip will be shown until GPU data is available
			const uint32_t least_detailed_lod = lod_count - 1;
			GPUPageAllocator::Page& page = pages.back();
			page = page_allocator.allocate();
			SparseMap(page_allocator, page, 0, 0, least_detailed_lod);
		}
		else
		{
			// For small/distant textures, we just map each tile up front and not manage residencies:
			uint32_t l_offset = 0;
			for (uint32_t lod = 0; lod < lod_count; ++lod)
			{
				const uint32_t l_width = std::max(1u, width >> lod);
				const uint32_t l_height = std::max(1u, height >> lod);
				for (uint32_t y = 0; y < l_height; ++y)
				{
					for (uint32_t x = 0; x < l_width; ++x)
					{
						const uint32_t l_index = l_offset + x + y * l_width;
						GPUPageAllocator::Page& page = pages[l_index];
						page = page_allocator.allocate();
						SparseMap(page_allocator, page, x, y, lod);
					}
				}
				l_offset += l_width * l_height;
			}
		}
		SparseFlush(page_allocator);
	}
	void VirtualTexture::DrawDebug(CommandList cmd)
	{
#ifdef TERRAIN_VIRTUAL_TEXTURE_DEBUG
		if (lod_count == 0)
			return;
		float cellsize = 20;
		XMFLOAT2 offset = XMFLOAT2(0, 20);

		const SubresourceData* feedback_data = feedbackMap_CPU_readback[cpu_resource_id].mapped_subresources;
		const SubresourceData* residency_data = residencyMap_CPU_readback[cpu_resource_id].mapped_subresources;

		const uint32_t width = feedbackMap.desc.width;
		const uint32_t height = feedbackMap.desc.height;

		uint32_t page_count = 0;
		uint32_t lod_offsets[9] = {};
		for (uint32_t i = 0; i < lod_count; ++i)
		{
			const uint32_t l_width = std::max(1u, width >> i);
			const uint32_t l_height = std::max(1u, height >> i);
			lod_offsets[i] = page_count;
			page_count += l_width * l_height;
		}

		{
			wi::font::Params font_params;
			font_params.shadowColor = wi::Color::Black();
			font_params.shadow_softness = 1;
			font_params.size = 20;
			font_params.posX = offset.x;
			font_params.posY = offset.y;
			std::string ss;
			ss += "Allocation count: " + std::to_string(tile_allocation_count) + "\n";
			ss += "Deallocation count: " + std::to_string(tile_deallocation_count) + "\n";
			wi::font::Draw(ss, font_params, cmd);
		}

		offset.y = 100;
		{
			wi::font::Params font_params;
			font_params.shadowColor = wi::Color::Black();
			font_params.shadow_softness = 1;
			font_params.size = 30;
			font_params.posX = offset.x;
			font_params.posY = offset.y - font_params.size;
			wi::font::Draw("GPU Tile Request:", font_params, cmd);
		}
		for (uint32_t y = 0; y < height; ++y)
		{
			for (uint32_t x = 0; x < width; ++x)
			{
				uint8_t* feedback_row = ((uint8_t*)feedback_data->data_ptr) + y * feedback_data->row_pitch;
				uint32_t feedback_request = *((uint32_t*)feedback_row + x);
				if (!data_available_CPU[cpu_resource_id])
					feedback_request = 0xFF;

				wi::image::Params image_params;
				image_params.pos.x = cellsize * x + offset.x;
				image_params.pos.y = cellsize * y + offset.y;
				image_params.siz.x = cellsize;
				image_params.siz.y = cellsize;
				image_params.blendFlag = wi::enums::BLENDMODE::BLENDMODE_ALPHA;
				image_params.opacity = 0.75f;
				if (feedback_request == 0xFF)
				{
					image_params.color = XMFLOAT4(0, 0, 0, 1);
				}
				else
				{
					image_params.color = XMFLOAT4(1.0f - wi::math::saturate((float)feedback_request / (float)lod_count), 0, 0, 1);
				}
				wi::image::Draw(wi::texturehelper::getWhite(), image_params, cmd);

				wi::font::Params font_params;
				font_params.position.x = cellsize * x + cellsize * 0.5f + offset.x;
				font_params.position.y = cellsize * y + cellsize * 0.5f + offset.y;
				font_params.h_align = wi::font::WIFALIGN_CENTER;
				font_params.v_align = wi::font::WIFALIGN_CENTER;
				if (feedback_request == 0xFF)
				{
					wi::font::Draw("X", font_params, cmd);
				}
				else
				{
					wi::font::Draw(std::to_string(feedback_request), font_params, cmd);
				}
			}
		}


		offset.x += cellsize * width + 30;
		{
			wi::font::Params font_params;
			font_params.shadowColor = wi::Color::Black();
			font_params.shadow_softness = 1;
			font_params.size = 30;
			font_params.posX = offset.x;
			font_params.posY = offset.y - font_params.size;
			wi::font::Draw("Min request per mip level:", font_params, cmd);
		}
		for (uint8_t lod = 0; lod < (uint8_t)lod_count; ++lod)
		{
			const uint32_t l_width = std::max(1u, width >> lod);
			const uint32_t l_height = std::max(1u, height >> lod);
			const uint32_t l_page_offset = lod_offsets[lod];

			for (uint32_t y = 0; y < l_height; ++y)
			{
				for (uint32_t x = 0; x < l_width; ++x)
				{
					const uint32_t l_index = l_page_offset + x + y * l_width;
					const uint32_t request = ((const uint32_t*)requestBuffer_CPU_readback[cpu_resource_id].mapped_data)[l_index];

					wi::image::Params image_params;
					image_params.pos.x = cellsize * x + offset.x;
					image_params.pos.y = cellsize * y + offset.y;
					image_params.siz.x = cellsize;
					image_params.siz.y = cellsize;
					image_params.blendFlag = wi::enums::BLENDMODE::BLENDMODE_ALPHA;
					image_params.opacity = 0.75f;
					if (request == 0xFF)
					{
						image_params.color = XMFLOAT4(0, 0, 0, 1);
					}
					else
					{
						image_params.color = XMFLOAT4(0, 0, 1, 1);
					}
					wi::image::Draw(wi::texturehelper::getWhite(), image_params, cmd);

					wi::font::Params font_params;
					font_params.position.x = cellsize * x + cellsize * 0.5f + offset.x;
					font_params.position.y = cellsize * y + cellsize * 0.5f + offset.y;
					font_params.h_align = wi::font::WIFALIGN_CENTER;
					font_params.v_align = wi::font::WIFALIGN_CENTER;
					if (request == 0xFF)
					{
						wi::font::Draw("X", font_params, cmd);
					}
					else
					{
						wi::font::Draw(std::to_string(request), font_params, cmd);
					}
				}
			}
			offset.x += cellsize * l_width + 30;
		}


		//////////////////////////////////////////////

		offset.x = 0;
		offset.y += cellsize * height + 30;
		{
			wi::font::Params font_params;
			font_params.shadowColor = wi::Color::Black();
			font_params.shadow_softness = 1;
			font_params.size = 30;
			font_params.posX = offset.x;
			font_params.posY = offset.y - font_params.size;
			wi::font::Draw("Min Resident Mip:", font_params, cmd);
		}
		for (uint32_t y = 0; y < height; ++y)
		{
			for (uint32_t x = 0; x < width; ++x)
			{
				uint8_t* residency_row = ((uint8_t*)residency_data->data_ptr) + y * residency_data->row_pitch;
				uint8_t lod = residency_row[x];

				wi::image::Params image_params;
				image_params.pos.x = cellsize * x + offset.x;
				image_params.pos.y = cellsize * y + offset.y;
				image_params.siz.x = cellsize;
				image_params.siz.y = cellsize;
				image_params.blendFlag = wi::enums::BLENDMODE::BLENDMODE_ALPHA;
				image_params.opacity = 0.75f;
				if (lod == 0xFF)
				{
					image_params.color = XMFLOAT4(0, 0, 0, 1);
				}
				else
				{
					image_params.color = XMFLOAT4(0, 1.0f - wi::math::saturate((float)lod / (float)lod_count), 0, 1);
				}
				wi::image::Draw(wi::texturehelper::getWhite(), image_params, cmd);

				wi::font::Params font_params;
				font_params.position.x = cellsize * x + cellsize * 0.5f + offset.x;
				font_params.position.y = cellsize * y + cellsize * 0.5f + offset.y;
				font_params.h_align = wi::font::WIFALIGN_CENTER;
				font_params.v_align = wi::font::WIFALIGN_CENTER;
				if (lod == 0xFF)
				{
					wi::font::Draw("X", font_params, cmd);
				}
				else
				{
					wi::font::Draw(std::to_string(lod), font_params, cmd);
				}
			}
		}

		offset.x += cellsize * width + 30;
		{
			wi::font::Params font_params;
			font_params.shadowColor = wi::Color::Black();
			font_params.shadow_softness = 1;
			font_params.size = 30;
			font_params.posX = offset.x;
			font_params.posY = offset.y - font_params.size;
			wi::font::Draw("Resident pages per mip level:", font_params, cmd);
		}
		for (uint8_t lod = 0; lod < (uint8_t)lod_count; ++lod)
		{
			const uint32_t l_width = std::max(1u, width >> lod);
			const uint32_t l_height = std::max(1u, height >> lod);
			const uint32_t l_page_offset = lod_offsets[lod];

			for (uint32_t y = 0; y < l_height; ++y)
			{
				for (uint32_t x = 0; x < l_width; ++x)
				{
					const uint32_t l_index = l_page_offset + x + y * l_width;
					auto page = pages[l_index];

					wi::image::Params image_params;
					image_params.pos.x = cellsize * x + offset.x;
					image_params.pos.y = cellsize * y + offset.y;
					image_params.siz.x = cellsize;
					image_params.siz.y = cellsize;
					image_params.blendFlag = wi::enums::BLENDMODE::BLENDMODE_ALPHA;
					image_params.opacity = 0.75f;
					if (page.IsValid())
					{
						image_params.color = XMFLOAT4(0, 0, 1, 1);
					}
					else
					{
						image_params.color = XMFLOAT4(0, 0, 0, 1);
					}
					wi::image::Draw(wi::texturehelper::getWhite(), image_params, cmd);

					wi::font::Params font_params;
					font_params.position.x = cellsize * x + cellsize * 0.5f + offset.x;
					font_params.position.y = cellsize * y + cellsize * 0.5f + offset.y;
					font_params.h_align = wi::font::WIFALIGN_CENTER;
					font_params.v_align = wi::font::WIFALIGN_CENTER;
					if (page.IsValid())
					{
						wi::font::Draw("O", font_params, cmd);
					}
					else
					{
						wi::font::Draw("X", font_params, cmd);
					}
				}
			}
			offset.x += cellsize * l_width + 30;
		}
#endif // TERRAIN_VIRTUAL_TEXTURE_DEBUG
	}
	void VirtualTexture::SparseMap(GPUPageAllocator& allocator, GPUPageAllocator::Page page, uint32_t x, uint32_t y, uint32_t mip)
	{
		if (page.IsValid() && page.block != last_block)
		{
			SparseFlush(allocator);
			last_block = page.block;
		}
		SparseResourceCoordinate& coordinate = sparse_coordinate.emplace_back();
		coordinate.x = x;
		coordinate.y = y;
		coordinate.mip = mip;
		SparseRegionSize& region = sparse_size.emplace_back();
		region.width = 1;
		region.height = 1;
		if (page.IsValid())
		{
			tile_range_flags.push_back(TileRangeFlags::None);
			tile_range_offset.push_back(page.index);

			// Request updating virtual texture tile after mapping:
			VirtualTexture::UpdateRequest& request = update_requests.emplace_back();
			request.tile_x = x;
			request.tile_y = y;
			request.lod = mip;

#ifdef TERRAIN_VIRTUAL_TEXTURE_DEBUG
			tile_allocation_count++;
#endif // TERRAIN_VIRTUAL_TEXTURE_DEBUG
		}
		else
		{
			tile_range_flags.push_back(TileRangeFlags::Null);
			tile_range_offset.push_back(0);

#ifdef TERRAIN_VIRTUAL_TEXTURE_DEBUG
			tile_deallocation_count++;
#endif // TERRAIN_VIRTUAL_TEXTURE_DEBUG
		}
		tile_range_count.push_back(1);
	}
	void VirtualTexture::SparseFlush(GPUPageAllocator& allocator)
	{
		if (sparse_coordinate.empty())
			return;
		SparseUpdateCommand command;
		command.sparse_resource = &texture;
		command.num_resource_regions = (uint32_t)sparse_coordinate.size();
		command.coordinates = sparse_coordinate.data();
		command.sizes = sparse_size.data();
		command.tile_pool = &allocator.blocks[last_block];
		command.range_flags = tile_range_flags.data();
		command.range_start_offsets = tile_range_offset.data();
		command.range_tile_counts = tile_range_count.data();
		GetDevice()->SparseUpdate(QUEUE_COPY, &command, 1);
		sparse_coordinate.clear();
		sparse_size.clear();
		tile_range_flags.clear();
		tile_range_offset.clear();
		tile_range_count.clear();
	}

	Terrain::Terrain()
	{
		weather.ambient = XMFLOAT3(0.4f, 0.4f, 0.4f);
		weather.SetRealisticSky(true);
		weather.SetVolumetricClouds(true);
		weather.volumetricCloudParameters.CoverageAmount = 0.65f;
		weather.volumetricCloudParameters.CoverageMinimum = 0.15f;
		weather.oceanParameters.waterHeight = -40;
		weather.oceanParameters.wave_amplitude = 120;
		weather.fogStart = 300;
		weather.fogEnd = 100000;
		weather.SetHeightFog(true);
		weather.fogHeightStart = 0;
		weather.fogHeightEnd = 100;
		weather.windDirection = XMFLOAT3(0.05f, 0.05f, 0.05f);
		weather.windSpeed = 4;
		weather.stars = 0.6f;

		grass_properties.viewDistance = chunk_width;

		generator = std::make_shared<Generator>();
	}
	Terrain::~Terrain()
	{
		Generation_Cancel();
	}

	void Terrain::Generation_Restart()
	{
		SetGenerationStarted(true);
		Generation_Cancel();
		generator->scene.Clear();

		chunks.clear();
		page_allocator = {};

		wi::vector<Entity> entities_to_remove;
		for (size_t i = 0; i < scene->hierarchy.GetCount(); ++i)
		{
			const HierarchyComponent& hier = scene->hierarchy[i];
			if (hier.parentID == terrainEntity)
			{
				entities_to_remove.push_back(scene->hierarchy.GetEntity(i));
			}
		}
		for (Entity x : entities_to_remove)
		{
			scene->Entity_Remove(x);
		}

		perlin_noise.init(seed);
		for (auto& modifier : modifiers)
		{
			modifier->Seed(seed);
		}

		// Add some nice weather and lighting:
		if (!scene->weathers.Contains(terrainEntity))
		{
			scene->weathers.Create(terrainEntity);
		}
		*scene->weathers.GetComponent(terrainEntity) = weather;
		if (!weather.IsOceanEnabled())
		{
			scene->ocean = {};
		}

		{
			Entity sunEntity = scene->Entity_CreateLight("terrainSun");
			scene->Component_Attach(sunEntity, terrainEntity);
			LightComponent& light = *scene->lights.GetComponent(sunEntity);
			light.SetType(LightComponent::LightType::DIRECTIONAL);
			light.intensity = 16;
			light.SetCastShadow(true);
			TransformComponent& transform = *scene->transforms.GetComponent(sunEntity);
			transform.RotateRollPitchYaw(XMFLOAT3(XM_PIDIV4, 0, XM_PIDIV4));
			transform.Translate(XMFLOAT3(0, 4, 0));
		}
	}

	void Terrain::Generation_Update(const CameraComponent& camera)
	{
		// The generation task is always cancelled every frame so we are sure that generation is not running at this point
		Generation_Cancel();

		if (!IsGenerationStarted())
		{
			Generation_Restart();
		}

		// Check whether any modifiers need to be removed, and we will really remove them here if so:
		if (!modifiers_to_remove.empty())
		{
			for (auto& modifier : modifiers_to_remove)
			{
				for (auto it = modifiers.begin(); it != modifiers.end(); ++it)
				{
					if (it->get() == modifier)
					{
						modifiers.erase(it);
						break;
					}
				}
			}
			Generation_Restart();
			modifiers_to_remove.clear();
		}

		if (terrainEntity == INVALID_ENTITY)
		{
			chunks.clear();
			page_allocator = {};
			return;
		}

		WeatherComponent* weather_component = scene->weathers.GetComponent(terrainEntity);
		if (weather_component != nullptr)
		{
			weather = *weather_component; // feedback default weather
		}

		// What was generated, will be merged in to the main scene
		scene->Merge(generator->scene);

		const float chunk_scale_rcp = 1.0f / chunk_scale;

		if (IsCenterToCamEnabled())
		{
			center_chunk.x = (int)std::floor((camera.Eye.x + chunk_half_width) * chunk_width_rcp * chunk_scale_rcp);
			center_chunk.z = (int)std::floor((camera.Eye.z + chunk_half_width) * chunk_width_rcp * chunk_scale_rcp);
		}

		const int removal_threshold = generation + 2;
		GraphicsDevice* device = GetDevice();

		// Check whether there are any materials that would write to virtual textures:
		bool virtual_texture_any = false;
		bool virtual_texture_available[TEXTURESLOT_COUNT] = {};
		MaterialComponent* virtual_materials[4] = {
			&material_Base,
			&material_Slope,
			&material_LowAltitude,
			&material_HighAltitude,
		};
		for (auto& material : virtual_materials)
		{
			for (int i = 0; i < TEXTURESLOT_COUNT; ++i)
			{
				virtual_texture_available[i] = false;
				switch (i)
				{
				case MaterialComponent::BASECOLORMAP:
				case MaterialComponent::NORMALMAP:
				case MaterialComponent::SURFACEMAP:
					if (material->textures[i].resource.IsValid())
					{
						virtual_texture_available[i] = true;
						virtual_texture_any = true;
					}
					break;
				default:
					break;
				}
			}
		}
		virtual_texture_available[MaterialComponent::SURFACEMAP] = true; // this is always needed to bake individual material properties

		for (auto it = chunks.begin(); it != chunks.end();)
		{
			const Chunk& chunk = it->first;
			ChunkData& chunk_data = it->second;
			chunk_data.visible = camera.frustum.CheckSphere(chunk_data.sphere.center, chunk_data.sphere.radius);
			const int dist = std::max(std::abs(center_chunk.x - chunk.x), std::abs(center_chunk.z - chunk.z));

			if (wi::renderer::GetOcclusionCullingEnabled())
			{
				size_t object_index = scene->objects.GetIndex(chunk_data.entity);
				if (object_index < scene->occlusion_results_objects.size())
				{
					if (scene->occlusion_results_objects[object_index].IsOccluded())
					{
						chunk_data.visible = false;
					}
				}
			}

			// pointer refresh:
			MeshComponent* chunk_mesh = scene->meshes.GetComponent(chunk_data.entity);
			if (chunk_mesh != nullptr)
			{
				chunk_data.mesh_vertex_positions = chunk_mesh->vertex_positions.data();

#if 0
				// Test: remove off screen chunks
				//	But the problem is that shadow casters shouldn't be removed outside view
				if (chunk_visible && !chunk_mesh->generalBuffer.IsValid())
				{
					chunk_mesh->CreateRenderData();
				}
				else if (!chunk_visible && chunk_mesh->generalBuffer.IsValid())
				{
					chunk_mesh->DeleteRenderData();
				}
#endif
			}
			else
			{
				chunk_data.mesh_vertex_positions = nullptr;
			}

			// chunk removal:
			if (IsRemovalEnabled())
			{
				if (dist > removal_threshold)
				{
					for (int i = 0; i < arraysize(chunk_data.vt); ++i)
					{
						chunk_data.vt[i].free(page_allocator);
					}
					scene->Entity_Remove(it->second.entity);
					it = chunks.erase(it);
					continue; // don't increment iterator
				}
				else
				{
					// Grass patch removal:
					if (chunk_data.grass_entity != INVALID_ENTITY && (dist > 1 || !IsGrassEnabled()))
					{
						scene->Entity_Remove(chunk_data.grass_entity);
						chunk_data.grass_entity = INVALID_ENTITY; // grass can be generated here by generation thread...
					}

					// Prop removal:
					if (chunk_data.props_entity != INVALID_ENTITY && (dist > prop_generation || std::abs(chunk_data.prop_density_current - prop_density) > std::numeric_limits<float>::epsilon()))
					{
						scene->Entity_Remove(chunk_data.props_entity);
						chunk_data.props_entity = INVALID_ENTITY; // prop can be generated here by generation thread...
					}
				}
			}

			// Grass property update:
			if (chunk_data.visible && chunk_data.grass_entity != INVALID_ENTITY)
			{
				wi::HairParticleSystem* grass = scene->hairs.GetComponent(chunk_data.grass_entity);
				if (grass != nullptr)
				{
					chunk_data.grass_density_current = grass_density;
					grass->strandCount = uint32_t(chunk_data.grass.strandCount * chunk_data.grass_density_current);
					grass->length = grass_properties.length;
					grass->viewDistance = grass_properties.viewDistance;
				}
			}

			RigidBodyPhysicsComponent* rigidbody = scene->rigidbodies.GetComponent(chunk_data.entity);
			if (IsPhysicsEnabled())
			{
				const ObjectComponent* object = scene->objects.GetComponent(chunk_data.entity);
				const int lod_required = object == nullptr ? 0 : object->lod;

				if (rigidbody != nullptr)
				{
					if (rigidbody->mesh_lod != lod_required)
					{
						rigidbody->mesh_lod = lod_required;
						rigidbody->physicsobject = {}; // will be recreated
					}
					else
					{
						if (dist < physics_generation)
						{
							wi::physics::SetActivationState(*rigidbody, wi::physics::ActivationState::Active);
						}
						else
						{
							//scene->rigidbodies.Remove(chunk_data.entity);
							wi::physics::SetActivationState(*rigidbody, wi::physics::ActivationState::Inactive);
						}
					}
				}
				else
				{
					if (dist < physics_generation)
					{
						RigidBodyPhysicsComponent& newrigidbody = scene->rigidbodies.Create(chunk_data.entity);
						newrigidbody.shape = RigidBodyPhysicsComponent::TRIANGLE_MESH;
						//newrigidbody.SetDisableDeactivation(true);
						newrigidbody.SetKinematic(true);
						newrigidbody.mesh_lod = lod_required;
					}
				}
			}
			else
			{
				if (rigidbody != nullptr)
				{
					scene->rigidbodies.Remove(chunk_data.entity);
				}
			}

			it++;
		}

		if (virtual_texture_any)
		{
			UpdateVirtualTexturesCPU();
		}

		// Start the generation on a background thread and keep it running until the next frame
		wi::jobsystem::Execute(generator->workload, [=](wi::jobsystem::JobArgs args) {

			wi::Timer timer;
			bool generated_something = false;

			auto request_chunk = [&](int offset_x, int offset_z)
			{
				Chunk chunk = center_chunk;
				chunk.x += offset_x;
				chunk.z += offset_z;
				auto it = chunks.find(chunk);
				if (it == chunks.end() || it->second.entity == INVALID_ENTITY)
				{
					// Generate a new chunk:
					ChunkData& chunk_data = chunks[chunk];

					chunk_data.entity = generator->scene.Entity_CreateObject("chunk_" + std::to_string(chunk.x) + "_" + std::to_string(chunk.z));
					ObjectComponent& object = *generator->scene.objects.GetComponent(chunk_data.entity);
					object.lod_distance_multiplier = lod_multiplier;
					object.filterMask |= wi::enums::FILTER_NAVIGATION_MESH;
					generator->scene.Component_Attach(chunk_data.entity, terrainEntity);

					TransformComponent& transform = *generator->scene.transforms.GetComponent(chunk_data.entity);
					transform.ClearTransform();
					chunk_data.position = XMFLOAT3(float(chunk.x * (chunk_width - 1)) * chunk_scale, 0, float(chunk.z * (chunk_width - 1)) * chunk_scale);
					transform.Translate(chunk_data.position);
					transform.UpdateTransform();

					MaterialComponent& material = generator->scene.materials.Create(chunk_data.entity);
					// material params will be 1 because they will be created from only texture maps
					//	because region materials are blended together into one texture
					material.SetRoughness(1);
					material.SetMetalness(1);
					material.SetReflectance(1);

					MeshComponent& mesh = generator->scene.meshes.Create(chunk_data.entity);
					object.meshID = chunk_data.entity;
					mesh.indices = chunk_indices.indices;
					for (auto& lod : chunk_indices.lods)
					{
						mesh.subsets.emplace_back();
						mesh.subsets.back().materialID = chunk_data.entity;
						mesh.subsets.back().indexCount = lod.indexCount;
						mesh.subsets.back().indexOffset = lod.indexOffset;
					}
					mesh.subsets_per_lod = 1;
					mesh.vertex_positions.resize(vertexCount);
					mesh.vertex_normals.resize(vertexCount);
					mesh.vertex_uvset_0.resize(vertexCount);
					chunk_data.region_weights.resize(vertexCount);

					chunk_data.mesh_vertex_positions = mesh.vertex_positions.data();

					wi::HairParticleSystem grass = grass_properties;
					grass.vertex_lengths.resize(vertexCount);
					std::atomic<uint32_t> grass_valid_vertex_count{ 0 };

					// Shadow casting will only be enabled for sloped terrain chunks:
					std::atomic_bool slope_cast_shadow;
					slope_cast_shadow.store(false);

					// Do a parallel for loop over all the chunk's vertices and compute their properties:
					wi::jobsystem::context ctx;
					wi::jobsystem::Dispatch(ctx, vertexCount, chunk_width, [&](wi::jobsystem::JobArgs args) {
						uint32_t index = args.jobIndex;
						const float x = (float(index % chunk_width) - chunk_half_width) * chunk_scale;
						const float z = (float(index / chunk_width) - chunk_half_width) * chunk_scale;
						XMVECTOR corners[3];
						XMFLOAT2 corner_offsets[3] = {
							XMFLOAT2(0, 0),
							XMFLOAT2(1, 0),
							XMFLOAT2(0, 1),
						};
						for (int i = 0; i < arraysize(corners); ++i)
						{
							float height = 0;
							const XMFLOAT2 world_pos = XMFLOAT2(chunk_data.position.x + x + corner_offsets[i].x, chunk_data.position.z + z + corner_offsets[i].y);
							for (auto& modifier : modifiers)
							{
								modifier->Apply(world_pos, height);
							}
							height = wi::math::Lerp(bottomLevel, topLevel, height);
							corners[i] = XMVectorSet(world_pos.x, height, world_pos.y, 0);
						}
						const float height = XMVectorGetY(corners[0]);
						const XMVECTOR T = XMVectorSubtract(corners[2], corners[1]);
						const XMVECTOR B = XMVectorSubtract(corners[1], corners[0]);
						const XMVECTOR N = XMVector3Normalize(XMVector3Cross(T, B));
						XMFLOAT3 normal;
						XMStoreFloat3(&normal, N);

						const float slope_amount = 1.0f - wi::math::saturate(normal.y);
						if (slope_amount > 0.1f)
							slope_cast_shadow.store(true);

						const float region_base = 1;
						const float region_slope = std::pow(slope_amount, region1);
						const float region_low_altitude = bottomLevel == 0 ? 0 : std::pow(wi::math::saturate(wi::math::InverseLerp(0, bottomLevel, height)), region2);
						const float region_high_altitude = topLevel == 0 ? 0 : std::pow(wi::math::saturate(wi::math::InverseLerp(0, topLevel, height)), region3);

						XMFLOAT4 materialBlendWeights(region_base, 0, 0, 0);
						materialBlendWeights = wi::math::Lerp(materialBlendWeights, XMFLOAT4(0, 1, 0, 0), region_slope);
						materialBlendWeights = wi::math::Lerp(materialBlendWeights, XMFLOAT4(0, 0, 1, 0), region_low_altitude);
						materialBlendWeights = wi::math::Lerp(materialBlendWeights, XMFLOAT4(0, 0, 0, 1), region_high_altitude);
						const float weight_norm = 1.0f / (materialBlendWeights.x + materialBlendWeights.y + materialBlendWeights.z + materialBlendWeights.w);
						materialBlendWeights.x *= weight_norm;
						materialBlendWeights.y *= weight_norm;
						materialBlendWeights.z *= weight_norm;
						materialBlendWeights.w *= weight_norm;

						chunk_data.region_weights[index] = wi::Color::fromFloat4(materialBlendWeights);

						mesh.vertex_positions[index] = XMFLOAT3(x, height, z);
						mesh.vertex_normals[index] = normal;
						const XMFLOAT2 uv = XMFLOAT2(x * chunk_scale_rcp * chunk_width_rcp + 0.5f, z * chunk_scale_rcp * chunk_width_rcp + 0.5f);
						mesh.vertex_uvset_0[index] = uv;

						XMFLOAT3 vertex_pos(chunk_data.position.x + x, height, chunk_data.position.z + z);

						const float grass_noise_frequency = 0.1f;
						const float grass_noise = perlin_noise.compute(vertex_pos.x * grass_noise_frequency, vertex_pos.y * grass_noise_frequency, vertex_pos.z * grass_noise_frequency) * 0.5f + 0.5f;
						const float region_grass = std::pow(materialBlendWeights.x * (1 - materialBlendWeights.w), 8.0f) * grass_noise;
						if (region_grass > 0.1f)
						{
							grass_valid_vertex_count.fetch_add(1);
							grass.vertex_lengths[index] = region_grass;
						}
						else
						{
							grass.vertex_lengths[index] = 0;
						}
						});
					wi::jobsystem::Wait(ctx); // wait until chunk's vertex buffer is fully generated

					material.SetCastShadow(slope_cast_shadow.load());
					mesh.SetDoubleSidedShadow(slope_cast_shadow.load());

					wi::jobsystem::Execute(ctx, [&](wi::jobsystem::JobArgs args) {
						mesh.CreateRenderData();
						chunk_data.sphere.center = mesh.aabb.getCenter();
						chunk_data.sphere.center.x += chunk_data.position.x;
						chunk_data.sphere.center.y += chunk_data.position.y;
						chunk_data.sphere.center.z += chunk_data.position.z;
						chunk_data.sphere.radius = mesh.aabb.getRadius();
						});

					// If there were any vertices in this chunk that could be valid for grass, store the grass particle system:
					if (grass_valid_vertex_count.load() > 0)
					{
						chunk_data.grass = std::move(grass); // the grass will be added to the scene later, only when the chunk is close to the camera (center chunk's neighbors)
						chunk_data.grass.meshID = chunk_data.entity;
						chunk_data.grass.strandCount = uint32_t(grass_valid_vertex_count.load() * 3 * chunk_scale * chunk_scale); // chunk_scale * chunk_scale : grass density increases with squared amount with chunk scale (x*z)
					}

					// Create the textures for virtual texture update:
					CreateChunkRegionTexture(chunk_data);

					wi::jobsystem::Wait(ctx); // wait until mesh.CreateRenderData() async task finishes
					generated_something = true;
				}

				const int dist = std::max(std::abs(center_chunk.x - chunk.x), std::abs(center_chunk.z - chunk.z));

				// Grass patch placement:
				if (dist <= 1 && IsGrassEnabled())
				{
					it = chunks.find(chunk);
					if (it != chunks.end() && it->second.entity != INVALID_ENTITY)
					{
						ChunkData& chunk_data = it->second;
						if (chunk_data.grass_entity == INVALID_ENTITY && chunk_data.grass.meshID != INVALID_ENTITY)
						{
							// add patch for this chunk
							chunk_data.grass_entity = CreateEntity();
							wi::HairParticleSystem& grass = generator->scene.hairs.Create(chunk_data.grass_entity);
							grass = chunk_data.grass;
							chunk_data.grass_density_current = grass_density;
							grass.strandCount = uint32_t(grass.strandCount * chunk_data.grass_density_current);
							generator->scene.materials.Create(chunk_data.grass_entity) = material_GrassParticle;
							generator->scene.transforms.Create(chunk_data.grass_entity);
							generator->scene.names.Create(chunk_data.grass_entity) = "grass";
							generator->scene.Component_Attach(chunk_data.grass_entity, chunk_data.entity, true);
							generated_something = true;
						}
					}
				}

				// Prop placement:
				if (dist <= prop_generation)
				{
					it = chunks.find(chunk);
					if (it != chunks.end() && it->second.entity != INVALID_ENTITY)
					{
						ChunkData& chunk_data = it->second;

						if (chunk_data.props_entity == INVALID_ENTITY && chunk_data.mesh_vertex_positions != nullptr)
						{
							chunk_data.props_entity = CreateEntity();
							generator->scene.transforms.Create(chunk_data.props_entity);
							generator->scene.names.Create(chunk_data.props_entity) = "props";
							generator->scene.Component_Attach(chunk_data.props_entity, chunk_data.entity, true);
							chunk_data.prop_density_current = prop_density;

							std::mt19937 prop_rand;
							prop_rand.seed((uint32_t)chunk.compute_hash() ^ seed);

							for (const auto& prop : props)
							{
								if (prop.data.empty())
									continue;
								std::uniform_int_distribution<uint32_t> gen_distr(
									uint32_t(prop.min_count_per_chunk * chunk_data.prop_density_current),
									uint32_t(prop.max_count_per_chunk * chunk_data.prop_density_current)
								);
								int gen_count = gen_distr(prop_rand);
								for (int i = 0; i < gen_count; ++i)
								{
									std::uniform_real_distribution<float> float_distr(0.0f, 1.0f);
									std::uniform_int_distribution<uint32_t> ind_distr(0, chunk_indices.lods[0].indexCount / 3 - 1);
									uint32_t tri = ind_distr(prop_rand); // random triangle on the chunk mesh
									uint32_t ind0 = chunk_indices.indices[tri * 3 + 0];
									uint32_t ind1 = chunk_indices.indices[tri * 3 + 1];
									uint32_t ind2 = chunk_indices.indices[tri * 3 + 2];
									const XMFLOAT3& pos0 = chunk_data.mesh_vertex_positions[ind0];
									const XMFLOAT3& pos1 = chunk_data.mesh_vertex_positions[ind1];
									const XMFLOAT3& pos2 = chunk_data.mesh_vertex_positions[ind2];
									const XMFLOAT4 region0 = chunk_data.region_weights[ind0];
									const XMFLOAT4 region1 = chunk_data.region_weights[ind1];
									const XMFLOAT4 region2 = chunk_data.region_weights[ind2];
									// random barycentric coords on the triangle:
									float f = float_distr(prop_rand);
									float g = float_distr(prop_rand);
									if (f + g > 1)
									{
										f = 1 - f;
										g = 1 - g;
									}
									XMFLOAT3 vertex_pos;
									vertex_pos.x = pos0.x + f * (pos1.x - pos0.x) + g * (pos2.x - pos0.x);
									vertex_pos.y = pos0.y + f * (pos1.y - pos0.y) + g * (pos2.y - pos0.y);
									vertex_pos.z = pos0.z + f * (pos1.z - pos0.z) + g * (pos2.z - pos0.z);
									XMFLOAT4 region;
									region.x = region0.x + f * (region1.x - region0.x) + g * (region2.x - region0.x);
									region.y = region0.y + f * (region1.y - region0.y) + g * (region2.y - region0.y);
									region.z = region0.z + f * (region1.z - region0.z) + g * (region2.z - region0.z);
									region.w = region0.w + f * (region1.w - region0.w) + g * (region2.w - region0.w);

									const float noise = std::pow(perlin_noise.compute((vertex_pos.x + chunk_data.position.x) * prop.noise_frequency, vertex_pos.y * prop.noise_frequency, (vertex_pos.z + chunk_data.position.z) * prop.noise_frequency) * 0.5f + 0.5f, prop.noise_power);
									const float chance = std::pow(((float*)&region)[prop.region], prop.region_power) * noise;
									if (chance > prop.threshold)
									{
										wi::Archive archive = wi::Archive(prop.data.data());
										EntitySerializer seri;
										Entity entity = generator->scene.Entity_Serialize(
											archive,
											seri,
											INVALID_ENTITY,
											wi::scene::Scene::EntitySerializeFlags::RECURSIVE |
											wi::scene::Scene::EntitySerializeFlags::KEEP_INTERNAL_ENTITY_REFERENCES
										);
										NameComponent* name = generator->scene.names.GetComponent(entity);
										if (name != nullptr)
										{
											name->name += std::to_string(i);
										}
										TransformComponent* transform = generator->scene.transforms.GetComponent(entity);
										if (transform == nullptr)
										{
											transform = &generator->scene.transforms.Create(entity);
										}
										transform->translation_local = vertex_pos;
										transform->translation_local.y += wi::math::Lerp(prop.min_y_offset, prop.max_y_offset, float_distr(prop_rand));
										const float scaling = wi::math::Lerp(prop.min_size, prop.max_size, float_distr(prop_rand));
										transform->Scale(XMFLOAT3(scaling, scaling, scaling));
										transform->RotateRollPitchYaw(XMFLOAT3(0, XM_2PI * float_distr(prop_rand), 0));
										transform->SetDirty();
										transform->UpdateTransform();
										generator->scene.Component_Attach(entity, chunk_data.props_entity, true);
										generated_something = true;
									}
								}
							}
							if (!IsPhysicsEnabled())
							{
								generator->scene.rigidbodies.Clear();
							}
						}
					}
				}

				if (generated_something && timer.elapsed_milliseconds() > generation_time_budget_milliseconds)
				{
					generator->cancelled.store(true);
				}

			};

			// generate center chunk first:
			request_chunk(0, 0);
			if (generator->cancelled.load()) return;

			// then generate neighbor chunks in outward spiral:
			for (int growth = 0; growth < generation; ++growth)
			{
				const int side = 2 * (growth + 1);
				int x = -growth - 1;
				int z = -growth - 1;
				for (int i = 0; i < side; ++i)
				{
					request_chunk(x, z);
					if (generator->cancelled.load()) return;
					x++;
				}
				for (int i = 0; i < side; ++i)
				{
					request_chunk(x, z);
					if (generator->cancelled.load()) return;
					z++;
				}
				for (int i = 0; i < side; ++i)
				{
					request_chunk(x, z);
					if (generator->cancelled.load()) return;
					x--;
				}
				for (int i = 0; i < side; ++i)
				{
					request_chunk(x, z);
					if (generator->cancelled.load()) return;
					z--;
				}
			}

			});

	}

	void Terrain::Generation_Cancel()
	{
		if (generator == nullptr)
			return;
		generator->cancelled.store(true); // tell the generation thread that work must be stopped
		wi::jobsystem::Wait(generator->workload); // waits until generation thread exits
		generator->cancelled.store(false); // the next generation can run
	}

	void Terrain::CreateChunkRegionTexture(ChunkData& chunk_data)
	{
		if (!chunk_data.region_weights_texture.IsValid())
		{
			GraphicsDevice* device = GetDevice();
			TextureDesc desc;
			desc.width = (uint32_t)chunk_width;
			desc.height = (uint32_t)chunk_width;
			desc.format = Format::R8G8B8A8_UNORM;
			desc.bind_flags = BindFlag::SHADER_RESOURCE;
			SubresourceData data;
			data.data_ptr = chunk_data.region_weights.data();
			data.row_pitch = chunk_width * sizeof(chunk_data.region_weights[0]);
			bool success = device->CreateTexture(&desc, &data, &chunk_data.region_weights_texture);
			assert(success);
		}
	}

	void Terrain::UpdateVirtualTexturesCPU()
	{
		GraphicsDevice* device = GetDevice();
		virtual_textures_in_use.clear();
		virtual_texture_barriers_before_update.clear();
		virtual_texture_barriers_after_update.clear();
		virtual_texture_barriers_before_allocation.clear();
		virtual_texture_barriers_after_allocation.clear();

		if (!sampler.IsValid())
		{
			SamplerDesc samplerDesc;
			samplerDesc.filter = Filter::MIN_MAG_MIP_LINEAR;	// Anisotropic sampler is too expensive with the sparse texture system currently
			samplerDesc.address_u = TextureAddressMode::CLAMP;	// Clamp sampler is needed to not oversample on chunk boundaries
			samplerDesc.address_v = TextureAddressMode::CLAMP;
			samplerDesc.address_w = TextureAddressMode::CLAMP;
			bool success = device->CreateSampler(&samplerDesc, &sampler);
			assert(success);
		}

		wi::jobsystem::context ctx;
		for (auto& it : chunks)
		{
			const Chunk& chunk = it.first;
			ChunkData& chunk_data = it.second;
			const uint32_t dist = (uint32_t)std::max(std::abs(center_chunk.x - chunk.x), std::abs(center_chunk.z - chunk.z));

			MaterialComponent* material = scene->materials.GetComponent(chunk_data.entity);
			if (material == nullptr)
				continue;

			material->sampler_descriptor = device->GetDescriptorIndex(&sampler);

			// This should have been created on generation thread, but if not (serialized), create it last minute:
			CreateChunkRegionTexture(chunk_data);

			const uint32_t min_resolution = 128u;
#ifdef TERRAIN_VIRTUAL_TEXTURE_DEBUG
			const uint32_t max_resolution = 2048;
#else
			const uint32_t max_resolution = 16384u;
#endif // TERRAIN_VIRTUAL_TEXTURE_DEBUG
			const uint32_t required_resolution = dist < 2 ? max_resolution : min_resolution;

			chunk_data.vt.resize(3); // base, normal, surface
			for (uint32_t map_type = 0; map_type < chunk_data.vt.size(); ++map_type)
			{
				VirtualTexture& vt = chunk_data.vt[map_type];

				if (vt.texture.desc.width != required_resolution)
				{
					vt.free(page_allocator);

					TextureDesc desc;
					desc.width = required_resolution;
					desc.height = required_resolution;
					desc.misc_flags = ResourceMiscFlag::SPARSE;
					desc.bind_flags = BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
					desc.mip_levels = 0;
					desc.layout = ResourceState::SHADER_RESOURCE_COMPUTE;

					if (map_type == MaterialComponent::NORMALMAP)
					{
						desc.format = Format::R8G8_UNORM;
					}
					else
					{
						desc.format = Format::R8G8B8A8_UNORM;
					}
					vt.init(page_allocator, desc);

					material->textures[map_type].resource.SetTexture(vt.texture);
					material->textures[map_type].sparse_residencymap_descriptor = device->GetDescriptorIndex(&vt.residencyMap, SubresourceType::SRV);
					material->textures[map_type].sparse_feedbackmap_descriptor = device->GetDescriptorIndex(&vt.feedbackMap, SubresourceType::UAV);

					vt.region_weights_texture = chunk_data.region_weights_texture;
					vt.map_type = map_type;
				}

				material->textures[map_type].lod_clamp = (float)vt.lod_count - 1;
				virtual_textures_in_use.push_back(&vt);
				virtual_texture_barriers_before_update.push_back(GPUBarrier::Image(&vt.texture, vt.texture.desc.layout, ResourceState::UNORDERED_ACCESS));
				virtual_texture_barriers_after_update.push_back(GPUBarrier::Image(&vt.texture, ResourceState::UNORDERED_ACCESS, vt.texture.desc.layout));

				if (!vt.residencyMap.IsValid())
					continue;

				// Process each virtual texture on a background thread:
				wi::jobsystem::Execute(ctx, [this, &vt](wi::jobsystem::JobArgs args) {

					const uint32_t width = vt.feedbackMap.desc.width;
					const uint32_t height = vt.feedbackMap.desc.height;

					// We must only access persistently mapped resources by CPU that the GPU is not using currently:
					vt.cpu_resource_id = (vt.cpu_resource_id + 1) % arraysize(vt.allocationBuffer_CPU_readback);
					const bool data_available_CPU = vt.data_available_CPU[vt.cpu_resource_id]; // indicates whether any GPU data is readable at this point or not
					vt.data_available_CPU[vt.cpu_resource_id] = true;

#ifdef TERRAIN_VIRTUAL_TEXTURE_DEBUG
					vt.tile_allocation_count = 0;
					vt.tile_deallocation_count = 0;
#endif // TERRAIN_VIRTUAL_TEXTURE_DEBUG

					// Perform the allocations and deallocations:
					//	GPU writes allocation requests by virtualTextureTileAllocateCS.hlsl compute shader
					if (vt.data_available_CPU)
					{
						uint32_t page_count = 0;
						uint32_t lod_offsets[9] = {};
						for (uint32_t i = 0; i < vt.lod_count; ++i)
						{
							const uint32_t l_width = std::max(1u, width >> i);
							const uint32_t l_height = std::max(1u, height >> i);
							lod_offsets[i] = page_count;
							page_count += l_width * l_height;
						}

						uint32_t allocation_count = *(const uint32_t*)vt.allocationBuffer_CPU_readback[vt.cpu_resource_id].mapped_data;
						allocation_count = std::min(uint32_t(vt.pages.size() - 1), allocation_count);
						//allocation_count = std::min(100u, allocation_count);
						const uint32_t* allocation_requests = ((const uint32_t*)vt.allocationBuffer_CPU_readback[vt.cpu_resource_id].mapped_data) + 1; // +1 offset of the allocation counter
						for (uint32_t i = 0; i < allocation_count; ++i)
						{
							const uint32_t allocation_request = allocation_requests[i];
							const uint8_t x = (allocation_request >> 24u) & 0xFF;
							const uint8_t y = (allocation_request >> 16u) & 0xFF;
							const uint8_t lod = (allocation_request >> 8u) & 0xFF;
							const bool allocate = allocation_request & 0x1;
							const bool must_be_always_resident = (int)lod == ((int)vt.lod_count - 1);
							if (lod >= vt.lod_count)
								continue;
							const uint32_t l_offset = lod_offsets[lod];
							const uint32_t l_width = std::max(1u, width >> lod);
							const uint32_t l_height = std::max(1u, height >> lod);
							const uint32_t l_index = l_offset + x + y * l_width;
							if (x >= l_width || y >= l_height)
								continue;
							GPUPageAllocator::Page& page = vt.pages[l_index];

							if (allocate)
							{
								if (page.IsValid())
								{
									continue;
								}
								page = page_allocator.allocate();
								if (page.IsValid())
								{
									vt.SparseMap(page_allocator, page, x, y, lod);
								}
							}
							else if(!must_be_always_resident)
							{
								if (!page.IsValid())
								{
									continue;
								}
								page_allocator.free(page);
								page = {};

								vt.SparseMap(page_allocator, page, x, y, lod); // invalid page will do unmap
							}
						}
					}

					// Update page buffer for GPU:
					uint32_t* page_buffer = (uint32_t*)vt.pageBuffer_CPU_upload[vt.cpu_resource_id].mapped_data;
					for (size_t i = 0; i < vt.pages.size(); ++i)
					{
						uint32_t page = (uint32_t)vt.pages[i].index;
						// force memcpy into uncached memory to avoid read stall by mistake:
						std::memcpy(page_buffer + i, &page, sizeof(page));
					}

					vt.SparseFlush(page_allocator);

				});

				virtual_texture_barriers_before_update.push_back(GPUBarrier::Buffer(&vt.pageBuffer, ResourceState::COPY_DST, ResourceState::SHADER_RESOURCE_COMPUTE));
				virtual_texture_barriers_before_update.push_back(GPUBarrier::Image(&vt.feedbackMap, vt.feedbackMap.desc.layout, ResourceState::UNORDERED_ACCESS));
				virtual_texture_barriers_before_update.push_back(GPUBarrier::Image(&vt.residencyMap, vt.residencyMap.desc.layout, ResourceState::UNORDERED_ACCESS));
				virtual_texture_barriers_after_update.push_back(GPUBarrier::Image(&vt.residencyMap, ResourceState::UNORDERED_ACCESS, vt.residencyMap.desc.layout));
				virtual_texture_barriers_before_allocation.push_back(GPUBarrier::Image(&vt.feedbackMap, ResourceState::UNORDERED_ACCESS, vt.feedbackMap.desc.layout));
				virtual_texture_barriers_after_allocation.push_back(GPUBarrier::Buffer(&vt.allocationBuffer, ResourceState::UNORDERED_ACCESS, ResourceState::COPY_SRC));
#ifdef TERRAIN_VIRTUAL_TEXTURE_DEBUG
				virtual_texture_barriers_after_allocation.push_back(GPUBarrier::Buffer(&vt.requestBuffer, ResourceState::UNORDERED_ACCESS, ResourceState::COPY_SRC));
#endif // TERRAIN_VIRTUAL_TEXTURE_DEBUG
			}
		}
		wi::jobsystem::Wait(ctx);
	}

	void Terrain::UpdateVirtualTexturesGPU(CommandList cmd) const
	{
		GraphicsDevice* device = GetDevice();
		device->EventBegin("Terrain - UpdateVirtualTexturesGPU", cmd);
		auto range = wi::profiler::BeginRangeGPU("Terrain - UpdateVirtualTexturesGPU", cmd);

		wi::renderer::ProcessDeferredMipGenRequests(cmd); // source textures should be mipmapped at this point

		device->Barrier(virtual_texture_barriers_before_update.data(), (uint32_t)virtual_texture_barriers_before_update.size(), cmd);

		device->EventBegin("Clear Metadata", cmd);
		for (const VirtualTexture* vt : virtual_textures_in_use)
		{
			if (!vt->residencyMap.IsValid())
				continue;
			device->ClearUAV(&vt->feedbackMap, 0xFF, cmd);
			device->ClearUAV(&vt->requestBuffer, 0xFF, cmd);
			device->ClearUAV(&vt->allocationBuffer, 0, cmd);
		}
		device->EventEnd(cmd);

		device->EventBegin("Update Residency Maps", cmd);
		device->BindComputeShader(wi::renderer::GetShader(wi::enums::CSTYPE_VIRTUALTEXTURE_RESIDENCYUPDATE), cmd);
		for (const VirtualTexture* vt : virtual_textures_in_use)
		{
			if (!vt->residencyMap.IsValid())
				continue;
			VirtualTextureResidencyUpdatePush push;
			push.lodCount = vt->lod_count;
			push.width = vt->residencyMap.desc.width;
			push.height = vt->residencyMap.desc.height;
			push.pageBufferRO = device->GetDescriptorIndex(&vt->pageBuffer, SubresourceType::SRV);
			push.residencyTextureRW = device->GetDescriptorIndex(&vt->residencyMap, SubresourceType::UAV);
			device->PushConstants(&push, sizeof(push), cmd);

			device->Dispatch(
				(vt->residencyMap.desc.width + 7u) / 8u,
				(vt->residencyMap.desc.height + 7u) / 8u,
				1u,
				cmd
			);
		}
		device->EventEnd(cmd);

		device->EventBegin("Render Tile Regions", cmd);
		device->BindComputeShader(wi::renderer::GetShader(wi::enums::CSTYPE_TERRAIN_VIRTUALTEXTURE_UPDATE), cmd);
		ShaderMaterial materials[4];
		material_Base.WriteShaderMaterial(&materials[0]);
		material_Slope.WriteShaderMaterial(&materials[1]);
		material_LowAltitude.WriteShaderMaterial(&materials[2]);
		material_HighAltitude.WriteShaderMaterial(&materials[3]);
		device->BindDynamicConstantBuffer(materials, 0, cmd);

		for (const VirtualTexture* vt : virtual_textures_in_use)
		{
			for (auto& request : vt->update_requests)
			{
				uint32_t first_mip = request.lod;
				uint32_t last_mip = request.lod;
				const bool packed_mips = request.lod >= vt->texture.sparse_properties->packed_mip_start;
				if (packed_mips && vt->texture.sparse_properties->packed_mip_count > 1)
				{
					first_mip = vt->texture.sparse_properties->packed_mip_start;
					last_mip = first_mip + vt->texture.sparse_properties->packed_mip_count - 1;
				}

				for (uint32_t mip = first_mip; mip <= last_mip; mip++)
				{
					const uint2 request_lod_resolution = uint2(
						vt->texture.desc.width >> mip,
						vt->texture.desc.height >> mip
					);

					const uint2 size = uint2(
						std::min(request_lod_resolution.x, vt->texture.sparse_properties->tile_width),
						std::min(request_lod_resolution.y, vt->texture.sparse_properties->tile_height)
					);

					TerrainVirtualTexturePush push;
					push.offset = uint2(
						request.tile_x * size.x,
						request.tile_y * size.y
					);
					push.map_type = vt->map_type;
					push.region_weights_textureRO = device->GetDescriptorIndex(&vt->region_weights_texture, SubresourceType::SRV);
					push.output_textureRW = device->GetDescriptorIndex(&vt->texture, SubresourceType::UAV, mip);
					device->PushConstants(&push, sizeof(push), cmd);

					device->Dispatch((size.x + 7u) / 8u, (size.y + 7u) / 8u, 1, cmd);
				}
			}
			vt->update_requests.clear();
		}
		device->EventEnd(cmd);

		device->Barrier(virtual_texture_barriers_after_update.data(), (uint32_t)virtual_texture_barriers_after_update.size(), cmd);

		wi::profiler::EndRange(range);
		device->EventEnd(cmd);
	}

	void Terrain::CopyVirtualTexturePageStatusGPU(CommandList cmd) const
	{
		GraphicsDevice* device = GetDevice();
		device->EventBegin("Terrain - CopyVirtualTexturePageStatusGPU", cmd);
		for (const VirtualTexture* vt : virtual_textures_in_use)
		{
			if (!vt->residencyMap.IsValid())
				continue;
			device->CopyResource(&vt->pageBuffer, &vt->pageBuffer_CPU_upload[vt->cpu_resource_id], cmd);
		}
		device->EventEnd(cmd);
	}

	void Terrain::AllocateVirtualTextureTileRequestsGPU(CommandList cmd) const
	{
		GraphicsDevice* device = GetDevice();

		device->EventBegin("Terrain - AllocateVirtualTextureTileRequestsGPU", cmd);
		device->Barrier(virtual_texture_barriers_before_allocation.data(), (uint32_t)virtual_texture_barriers_before_allocation.size(), cmd);

		{
			device->EventBegin("Tile Requests", cmd);
			device->BindComputeShader(wi::renderer::GetShader(wi::enums::CSTYPE_VIRTUALTEXTURE_TILEREQUESTS), cmd);
			for (const VirtualTexture* vt : virtual_textures_in_use)
			{
				if (!vt->residencyMap.IsValid())
					continue;
				VirtualTextureTileRequestsPush push;
				push.lodCount = vt->lod_count;
				push.width = vt->feedbackMap.desc.width;
				push.height = vt->feedbackMap.desc.height;
				push.feedbackTextureRO = device->GetDescriptorIndex(&vt->feedbackMap, SubresourceType::SRV);
				push.requestBufferRW = device->GetDescriptorIndex(&vt->requestBuffer, SubresourceType::UAV);
				device->PushConstants(&push, sizeof(push), cmd);

				device->Dispatch(
					(std::max(1u, vt->feedbackMap.desc.width / 2) + 7u) / 8u,
					(std::max(1u, vt->feedbackMap.desc.height / 2) + 7u) / 8u,
					1u,
					cmd
				);
			}
			device->EventEnd(cmd);
		}

		GPUBarrier memory_barrier = GPUBarrier::Memory();
		device->Barrier(&memory_barrier, 1, cmd);

		{
			device->EventBegin("Tile Allocation Requests", cmd);
			device->BindComputeShader(wi::renderer::GetShader(wi::enums::CSTYPE_VIRTUALTEXTURE_TILEALLOCATE), cmd);
			for (const VirtualTexture* vt : virtual_textures_in_use)
			{
				if (!vt->residencyMap.IsValid())
					continue;
				VirtualTextureTileAllocatePush push;
				push.threadCount = (uint)vt->pages.size();
				push.lodCount = vt->lod_count;
				push.width = vt->feedbackMap.desc.width;
				push.height = vt->feedbackMap.desc.height;
				push.pageBufferRO = device->GetDescriptorIndex(&vt->pageBuffer, SubresourceType::SRV);
				push.requestBufferRW = device->GetDescriptorIndex(&vt->requestBuffer, SubresourceType::UAV);
				push.allocationBufferRW = device->GetDescriptorIndex(&vt->allocationBuffer, SubresourceType::UAV);
				device->PushConstants(&push, sizeof(push), cmd);

				device->Dispatch(
					(push.threadCount + 63u) / 64u,
					1u,
					1u,
					cmd
				);
			}
			device->EventEnd(cmd);
		}

		device->Barrier(virtual_texture_barriers_after_allocation.data(), (uint32_t)virtual_texture_barriers_after_allocation.size(), cmd);

		device->EventEnd(cmd);
	}

	void Terrain::WritebackTileRequestsGPU(CommandList cmd) const
	{
		GraphicsDevice* device = GetDevice();

		device->EventBegin("Terrain - WritebackTileRequestsGPU", cmd);

		for (const VirtualTexture* vt : virtual_textures_in_use)
		{
			if (!vt->residencyMap.IsValid())
				continue;
			device->CopyResource(&vt->allocationBuffer_CPU_readback[vt->cpu_resource_id], &vt->allocationBuffer, cmd);

#ifdef TERRAIN_VIRTUAL_TEXTURE_DEBUG
			device->CopyResource(&vt->feedbackMap_CPU_readback[vt->cpu_resource_id], &vt->feedbackMap, cmd);
			device->CopyResource(&vt->residencyMap_CPU_readback[vt->cpu_resource_id], &vt->residencyMap, cmd);
			device->CopyResource(&vt->requestBuffer_CPU_readback[vt->cpu_resource_id], &vt->requestBuffer, cmd);
#endif // TERRAIN_VIRTUAL_TEXTURE_DEBUG
		}

		device->EventEnd(cmd);
	}

	void Terrain::Serialize(wi::Archive& archive, wi::ecs::EntitySerializer& seri)
	{
		Generation_Cancel();

		if (archive.IsReadMode())
		{
			archive >> _flags;
			archive >> lod_multiplier;
			if (seri.GetVersion() < 3)
			{
				float texlod;
				archive >> texlod;
			}
			archive >> generation;
			archive >> prop_generation;
			archive >> prop_density;
			archive >> grass_density;
			archive >> chunk_scale;
			archive >> seed;
			archive >> bottomLevel;
			archive >> topLevel;
			archive >> region1;
			archive >> region2;
			archive >> region3;

			archive >> center_chunk.x;
			archive >> center_chunk.z;

			if (seri.GetVersion() >= 1)
			{
				archive >> physics_generation;
			}
			if (seri.GetVersion() >= 2 && seri.GetVersion() < 3)
			{
				uint32_t target_texture_resolution;
				archive >> target_texture_resolution;
			}

			size_t count = 0;
			archive >> count;
			props.resize(count);
			for (size_t i = 0; i < props.size(); ++i)
			{
				Prop& prop = props[i];
				if (seri.GetVersion() >= 1)
				{
					archive >> prop.data;

					if (!prop.data.empty())
					{
						// Serialize the prop data in read mode and remap internal entity references:
						Scene tmp_scene;
						wi::Archive tmp_archive = wi::Archive(prop.data.data());
						Entity entity = tmp_scene.Entity_Serialize(
							tmp_archive,
							seri,
							INVALID_ENTITY,
							wi::scene::Scene::EntitySerializeFlags::RECURSIVE
						);

						// Serialize again with the remapped references:
						wi::Archive remapped_archive;
						remapped_archive.SetReadModeAndResetPos(false);
						tmp_scene.Entity_Serialize(
							remapped_archive,
							seri,
							entity,
							wi::scene::Scene::EntitySerializeFlags::RECURSIVE
						);

						// Replace original data with remapped references for current session:
						remapped_archive.WriteData(prop.data);
					}
				}
				else
				{
					// Back compat reading of terrain version 0:
					std::string name;
					Entity mesh_entity;
					ObjectComponent object;
					archive >> name;
					SerializeEntity(archive, mesh_entity, seri);
					object.Serialize(archive, seri);
					Scene tmp_scene;
					Entity object_entity = CreateEntity();
					tmp_scene.names.Create(object_entity) = name;
					tmp_scene.objects.Create(object_entity) = object;
					tmp_scene.transforms.Create(object_entity);
					wi::Archive archive;
					EntitySerializer seri;
					tmp_scene.Entity_Serialize(
						archive,
						seri,
						object_entity,
						wi::scene::Scene::EntitySerializeFlags::RECURSIVE | wi::scene::Scene::EntitySerializeFlags::KEEP_INTERNAL_ENTITY_REFERENCES
					);
					archive.WriteData(prop.data);
				}
				archive >> prop.min_count_per_chunk;
				archive >> prop.max_count_per_chunk;
				archive >> prop.region;
				archive >> prop.region_power;
				archive >> prop.noise_frequency;
				archive >> prop.noise_power;
				archive >> prop.threshold;
				archive >> prop.min_size;
				archive >> prop.max_size;
				archive >> prop.min_y_offset;
				archive >> prop.max_y_offset;
			}

			archive >> count;
			chunks.reserve(count);
			for (size_t i = 0; i < count; ++i)
			{
				Chunk chunk;
				archive >> chunk.x;
				archive >> chunk.z;
				ChunkData& chunk_data = chunks[chunk];
				SerializeEntity(archive, chunk_data.entity, seri);
				SerializeEntity(archive, chunk_data.grass_entity, seri);
				SerializeEntity(archive, chunk_data.props_entity, seri);
				archive >> chunk_data.prop_density_current;
				chunk_data.grass.Serialize(archive, seri);
				archive >> chunk_data.grass_density_current;
				archive >> chunk_data.region_weights;
				archive >> chunk_data.sphere.center;
				archive >> chunk_data.sphere.radius;
				archive >> chunk_data.position;
			}

			archive >> count;
			modifiers.resize(count);
			for (size_t i = 0; i < modifiers.size(); ++i)
			{
				uint32_t value;
				archive >> value;
				Modifier::Type type = (Modifier::Type)value;

				switch (type)
				{
				default:
				case Modifier::Type::Perlin:
					{
						std::shared_ptr<PerlinModifier> modifier = std::make_shared<PerlinModifier>();
						modifiers[i] = modifier;
						archive >> modifier->octaves;
						archive >> modifier->seed;
						modifier->perlin_noise.Serialize(archive);
					}
					break;
				case Modifier::Type::Voronoi:
					{
						std::shared_ptr<VoronoiModifier> modifier = std::make_shared<VoronoiModifier>();
						modifiers[i] = modifier;
						archive >> modifier->fade;
						archive >> modifier->shape;
						archive >> modifier->falloff;
						archive >> modifier->perturbation;
						archive >> modifier->seed;
						modifier->perlin_noise.Serialize(archive);
					}
					break;
				case Modifier::Type::Heightmap:
					{
						std::shared_ptr<HeightmapModifier> modifier = std::make_shared<HeightmapModifier>();
						modifiers[i] = modifier;
						archive >> modifier->scale;
						archive >> modifier->data;
						archive >> modifier->width;
						archive >> modifier->height;
					}
					break;
				}

				Modifier* modifier = modifiers[i].get();
				archive >> value;
				modifier->blend = (Modifier::BlendMode)value;
				archive >> modifier->weight;
				archive >> modifier->frequency;
			}

		}
		else
		{
			archive << _flags;
			archive << lod_multiplier;
			if (seri.GetVersion() < 3)
			{
				float texlod = 1;
				archive << texlod;
			}
			archive << generation;
			archive << prop_generation;
			archive << prop_density;
			archive << grass_density;
			archive << chunk_scale;
			archive << seed;
			archive << bottomLevel;
			archive << topLevel;
			archive << region1;
			archive << region2;
			archive << region3;

			archive << center_chunk.x;
			archive << center_chunk.z;

			if (seri.GetVersion() >= 1)
			{
				archive << physics_generation;
			}
			if (seri.GetVersion() >= 2 && seri.GetVersion() < 3)
			{
				uint32_t target_texture_resolution = 1024;
				archive << target_texture_resolution;
			}

			archive << props.size();
			for (size_t i = 0; i < props.size(); ++i)
			{
				Prop& prop = props[i];
				archive << prop.data;
				archive << prop.min_count_per_chunk;
				archive << prop.max_count_per_chunk;
				archive << prop.region;
				archive << prop.region_power;
				archive << prop.noise_frequency;
				archive << prop.noise_power;
				archive << prop.threshold;
				archive << prop.min_size;
				archive << prop.max_size;
				archive << prop.min_y_offset;
				archive << prop.max_y_offset;
			}

			archive << chunks.size();
			for (auto& it : chunks)
			{
				const Chunk& chunk = it.first;
				archive << chunk.x;
				archive << chunk.z;
				ChunkData& chunk_data = it.second;
				SerializeEntity(archive, chunk_data.entity, seri);
				SerializeEntity(archive, chunk_data.grass_entity, seri);
				SerializeEntity(archive, chunk_data.props_entity, seri);
				archive << chunk_data.prop_density_current;
				chunk_data.grass.Serialize(archive, seri);
				archive << chunk_data.grass_density_current;
				archive << chunk_data.region_weights;
				archive << chunk_data.sphere.center;
				archive << chunk_data.sphere.radius;
				archive << chunk_data.position;
			}

			archive << modifiers.size();
			for (auto& modifier : modifiers)
			{
				archive << (uint32_t)modifier->type;
				switch (modifier->type)
				{
				default:
				case Modifier::Type::Perlin:
					archive << ((PerlinModifier*)modifier.get())->octaves;
					archive << ((PerlinModifier*)modifier.get())->seed;
					((PerlinModifier*)modifier.get())->perlin_noise.Serialize(archive);
					break;
				case Modifier::Type::Voronoi:
					archive << ((VoronoiModifier*)modifier.get())->fade;
					archive << ((VoronoiModifier*)modifier.get())->shape;
					archive << ((VoronoiModifier*)modifier.get())->falloff;
					archive << ((VoronoiModifier*)modifier.get())->perturbation;
					archive << ((VoronoiModifier*)modifier.get())->seed;
					((VoronoiModifier*)modifier.get())->perlin_noise.Serialize(archive);
					break;
				case Modifier::Type::Heightmap:
					archive << ((HeightmapModifier*)modifier.get())->scale;
					archive << ((HeightmapModifier*)modifier.get())->data;
					archive << ((HeightmapModifier*)modifier.get())->width;
					archive << ((HeightmapModifier*)modifier.get())->height;
					break;
				}

				archive << (uint32_t)modifier->blend;
				archive << modifier->weight;
				archive << modifier->frequency;
			}
		}

		material_Base.Serialize(archive, seri);
		material_Slope.Serialize(archive, seri);
		material_LowAltitude.Serialize(archive, seri);
		material_HighAltitude.Serialize(archive, seri);
		material_GrassParticle.Serialize(archive, seri);
		weather.Serialize(archive, seri);
		grass_properties.Serialize(archive, seri);
		perlin_noise.Serialize(archive);
	}

}
