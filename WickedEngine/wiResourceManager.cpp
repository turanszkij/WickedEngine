#include "wiResourceManager.h"
#include "wiRenderer.h"
#include "wiHelper.h"
#include "wiTextureHelper.h"
#include "wiUnorderedMap.h"

#include "Utility/stb_image.h"
#include "Utility/tinyddsloader.h"
#include "Utility/basis_universal/transcoder/basisu_transcoder.h"
extern basist::etc1_global_selector_codebook g_basis_global_codebook;

#include <algorithm>
#include <mutex>

using namespace wiGraphics;

namespace wiResourceManager
{
	std::mutex locker;
	wi::unordered_map<std::string, std::weak_ptr<wiResource>> resources;
	MODE mode = MODE_DISCARD_FILEDATA_AFTER_LOAD;

	void SetMode(MODE param)
	{
		mode = param;
	}
	MODE GetMode()
	{
		return mode;
	}

	static const wi::unordered_map<std::string, wiResource::DATA_TYPE> types = {
		std::make_pair("BASIS", wiResource::IMAGE),
		std::make_pair("KTX2", wiResource::IMAGE),
		std::make_pair("JPG", wiResource::IMAGE),
		std::make_pair("JPEG", wiResource::IMAGE),
		std::make_pair("PNG", wiResource::IMAGE),
		std::make_pair("BMP", wiResource::IMAGE),
		std::make_pair("DDS", wiResource::IMAGE),
		std::make_pair("TGA", wiResource::IMAGE),
		std::make_pair("WAV", wiResource::SOUND),
		std::make_pair("OGG", wiResource::SOUND),
	};
	std::vector<std::string> GetSupportedImageExtensions()
	{
		std::vector<std::string> ret;
		for (auto& x : types)
		{
			if (x.second == wiResource::IMAGE)
			{
				ret.push_back(x.first);
			}
		}
		return ret;
	}
	std::vector<std::string> GetSupportedSoundExtensions()
	{
		std::vector<std::string> ret;
		for (auto& x : types)
		{
			if (x.second == wiResource::SOUND)
			{
				ret.push_back(x.first);
			}
		}
		return ret;
	}

	std::shared_ptr<wiResource> Load(const std::string& name, uint32_t flags, const uint8_t* filedata, size_t filesize)
	{
		if (mode == MODE_DISCARD_FILEDATA_AFTER_LOAD)
		{
			flags &= ~IMPORT_RETAIN_FILEDATA;
		}

		locker.lock();
		std::weak_ptr<wiResource>& weak_resource = resources[name];
		std::shared_ptr<wiResource> resource = weak_resource.lock();

		static bool basis_init = false; // within lock!
		if (!basis_init)
		{
			basis_init = true;
			basist::basisu_transcoder_init();
		}

		if (resource == nullptr)
		{
			resource = std::make_shared<wiResource>();
			resources[name] = resource;
			locker.unlock();
		}
		else
		{
			locker.unlock();
			return resource;
		}

		if (filedata == nullptr || filesize == 0)
		{
			if (!wiHelper::FileRead(name, resource->filedata))
			{
				resource.reset();
				return nullptr;
			}
			filedata = resource->filedata.data();
			filesize = resource->filedata.size();
		}

		std::string ext = wiHelper::toUpper(wiHelper::GetExtensionFromFileName(name));
		wiResource::DATA_TYPE type;

		// dynamic type selection:
		{
			auto it = types.find(ext);
			if (it != types.end())
			{
				type = it->second;
			}
			else
			{
				return nullptr;
			}
		}

		bool success = false;

		switch (type)
		{
		case wiResource::IMAGE:
		{
			GraphicsDevice* device = wiGraphics::GetDevice();
			if (!ext.compare("KTX2"))
			{
				basist::ktx2_transcoder transcoder(&g_basis_global_codebook);
				if (transcoder.init(filedata, (uint32_t)filesize))
				{
					TextureDesc desc;
					desc.bind_flags = BindFlag::SHADER_RESOURCE;
					desc.width = transcoder.get_width();
					desc.height = transcoder.get_height();
					desc.array_size = std::max(desc.array_size, transcoder.get_layers() * transcoder.get_faces());
					desc.mip_levels = transcoder.get_levels();
					if (transcoder.get_faces() == 6)
					{
						desc.misc_flags = ResourceMiscFlag::TEXTURECUBE;
					}

					basist::transcoder_texture_format fmt;
					if (transcoder.get_has_alpha())
					{
						fmt = basist::transcoder_texture_format::cTFBC3_RGBA;
						desc.format = Format::BC3_UNORM;
					}
					else
					{
						fmt = basist::transcoder_texture_format::cTFBC1_RGB;
						desc.format = Format::BC1_UNORM;
					}
					uint32_t bytes_per_block = basis_get_bytes_per_block_or_pixel(fmt);

					if (transcoder.start_transcoding())
					{
						// all subresources will use one allocation for transcoder destination, so compute combined size:
						size_t transcoded_data_size = 0;
						for (uint32_t layer = 0; layer < std::max(1u, transcoder.get_layers()); ++layer)
						{
							for (uint32_t face = 0; face < transcoder.get_faces(); ++face)
							{
								for (uint32_t mip = 0; mip < transcoder.get_levels(); ++mip)
								{
									basist::ktx2_image_level_info level_info;
									if (transcoder.get_image_level_info(level_info, mip, layer, face))
									{
										transcoded_data_size += level_info.m_total_blocks * bytes_per_block;
									}
								}
							}
						}
						std::vector<uint8_t*> transcoded_data(transcoded_data_size);

						std::vector<SubresourceData> InitData;
						size_t transcoded_data_offset = 0;
						for (uint32_t layer = 0; layer < std::max(1u, transcoder.get_layers()); ++layer)
						{
							for (uint32_t face = 0; face < transcoder.get_faces(); ++face)
							{
								for (uint32_t mip = 0; mip < transcoder.get_levels(); ++mip)
								{
									basist::ktx2_image_level_info level_info;
									if (transcoder.get_image_level_info(level_info, mip, layer, face))
									{
										void* data_ptr = transcoded_data.data() + transcoded_data_offset;
										transcoded_data_offset += level_info.m_total_blocks * bytes_per_block;
										if (transcoder.transcode_image_level(
											mip, layer, face,
											data_ptr,
											level_info.m_total_blocks,
											fmt
										))
										{
											SubresourceData subresourceData;
											subresourceData.data_ptr = data_ptr;
											subresourceData.row_pitch = level_info.m_num_blocks_x * bytes_per_block;
											subresourceData.slice_pitch = subresourceData.row_pitch * level_info.m_num_blocks_y;
											InitData.push_back(subresourceData);
										}
									}
								}
							}
						}

						if (!InitData.empty())
						{
							success = device->CreateTexture(&desc, InitData.data(), &resource->texture);
							device->SetName(&resource->texture, name.c_str());
						}
					}
					transcoder.clear();
				}
			}
			else if (!ext.compare("BASIS"))
			{
				basist::basisu_transcoder transcoder(&g_basis_global_codebook);
				if (transcoder.validate_header(filedata, (uint32_t)filesize))
				{
					basist::basisu_file_info fileInfo;
					if (transcoder.get_file_info(filedata, (uint32_t)filesize, fileInfo))
					{
						uint32_t image_index = 0;
						basist::basisu_image_info info;
						if (transcoder.get_image_info(filedata, (uint32_t)filesize, info, image_index))
						{
							TextureDesc desc;
							desc.bind_flags = BindFlag::SHADER_RESOURCE;
							desc.width = info.m_width;
							desc.height = info.m_height;
							desc.mip_levels = info.m_total_levels;

							basist::transcoder_texture_format fmt;
							if (info.m_alpha_flag)
							{
								fmt = basist::transcoder_texture_format::cTFBC3_RGBA;
								desc.format = Format::BC3_UNORM;
							}
							else
							{
								fmt = basist::transcoder_texture_format::cTFBC1_RGB;
								desc.format = Format::BC1_UNORM;
							}
							uint32_t bytes_per_block = basis_get_bytes_per_block_or_pixel(fmt);

							if (transcoder.start_transcoding(filedata, (uint32_t)filesize))
							{
								// all subresources will use one allocation for transcoder destination, so compute combined size:
								size_t transcoded_data_size = 0;
								for (uint32_t mip = 0; mip < desc.mip_levels; ++mip)
								{
									basist::basisu_image_level_info level_info;
									if (transcoder.get_image_level_info(filedata, (uint32_t)filesize, level_info, image_index, mip))
									{
										transcoded_data_size += level_info.m_total_blocks * bytes_per_block;
									}
								}
								std::vector<uint8_t*> transcoded_data(transcoded_data_size);

								std::vector<SubresourceData> InitData;
								size_t transcoded_data_offset = 0;
								for (uint32_t mip = 0; mip < desc.mip_levels; ++mip)
								{
									basist::basisu_image_level_info level_info;
									if (transcoder.get_image_level_info(filedata, (uint32_t)filesize, level_info, 0, mip))
									{
										void* data_ptr = transcoded_data.data() + transcoded_data_offset;
										transcoded_data_offset += level_info.m_total_blocks * bytes_per_block;
										if (transcoder.transcode_image_level(
											filedata, (uint32_t)filesize, image_index,
											mip,
											data_ptr,
											info.m_total_blocks,
											fmt
										))
										{
											SubresourceData subresourceData;
											subresourceData.data_ptr = data_ptr;
											subresourceData.row_pitch = level_info.m_num_blocks_x * bytes_per_block;
											subresourceData.slice_pitch = subresourceData.row_pitch * level_info.m_num_blocks_y;
											InitData.push_back(subresourceData);
										}
									}
								}

								if (!InitData.empty())
								{
									success = device->CreateTexture(&desc, InitData.data(), &resource->texture);
									device->SetName(&resource->texture, name.c_str());
								}
							}
						}
					}
				}
			}
			else if (!ext.compare("DDS"))
			{
				// Load dds

				tinyddsloader::DDSFile dds;
				auto result = dds.Load(filedata, filesize);

				if (result == tinyddsloader::Result::Success)
				{
					TextureDesc desc;
					desc.array_size = 1;
					desc.bind_flags = BindFlag::SHADER_RESOURCE;
					desc.width = dds.GetWidth();
					desc.height = dds.GetHeight();
					desc.depth = dds.GetDepth();
					desc.mip_levels = dds.GetMipCount();
					desc.array_size = dds.GetArraySize();
					desc.format = Format::R8G8B8A8_UNORM;
					desc.layout = ResourceState::SHADER_RESOURCE;

					if (dds.IsCubemap())
					{
						desc.misc_flags |= ResourceMiscFlag::TEXTURECUBE;
					}

					auto ddsFormat = dds.GetFormat();

					switch (ddsFormat)
					{
					case tinyddsloader::DDSFile::DXGIFormat::R32G32B32A32_Float: desc.format = Format::R32G32B32A32_FLOAT; break;
					case tinyddsloader::DDSFile::DXGIFormat::R32G32B32A32_UInt: desc.format = Format::R32G32B32A32_UINT; break;
					case tinyddsloader::DDSFile::DXGIFormat::R32G32B32A32_SInt: desc.format = Format::R32G32B32A32_SINT; break;
					case tinyddsloader::DDSFile::DXGIFormat::R32G32B32_Float: desc.format = Format::R32G32B32_FLOAT; break;
					case tinyddsloader::DDSFile::DXGIFormat::R32G32B32_UInt: desc.format = Format::R32G32B32_UINT; break;
					case tinyddsloader::DDSFile::DXGIFormat::R32G32B32_SInt: desc.format = Format::R32G32B32_SINT; break;
					case tinyddsloader::DDSFile::DXGIFormat::R16G16B16A16_Float: desc.format = Format::R16G16B16A16_FLOAT; break;
					case tinyddsloader::DDSFile::DXGIFormat::R16G16B16A16_UNorm: desc.format = Format::R16G16B16A16_UNORM; break;
					case tinyddsloader::DDSFile::DXGIFormat::R16G16B16A16_UInt: desc.format = Format::R16G16B16A16_UINT; break;
					case tinyddsloader::DDSFile::DXGIFormat::R16G16B16A16_SNorm: desc.format = Format::R16G16B16A16_SNORM; break;
					case tinyddsloader::DDSFile::DXGIFormat::R16G16B16A16_SInt: desc.format = Format::R16G16B16A16_SINT; break;
					case tinyddsloader::DDSFile::DXGIFormat::R32G32_Float: desc.format = Format::R32G32_FLOAT; break;
					case tinyddsloader::DDSFile::DXGIFormat::R32G32_UInt: desc.format = Format::R32G32_UINT; break;
					case tinyddsloader::DDSFile::DXGIFormat::R32G32_SInt: desc.format = Format::R32G32_SINT; break;
					case tinyddsloader::DDSFile::DXGIFormat::R10G10B10A2_UNorm: desc.format = Format::R10G10B10A2_UNORM; break;
					case tinyddsloader::DDSFile::DXGIFormat::R10G10B10A2_UInt: desc.format = Format::R10G10B10A2_UINT; break;
					case tinyddsloader::DDSFile::DXGIFormat::R11G11B10_Float: desc.format = Format::R11G11B10_FLOAT; break;
					case tinyddsloader::DDSFile::DXGIFormat::B8G8R8A8_UNorm: desc.format = Format::B8G8R8A8_UNORM; break;
					case tinyddsloader::DDSFile::DXGIFormat::B8G8R8A8_UNorm_SRGB: desc.format = Format::B8G8R8A8_UNORM_SRGB; break;
					case tinyddsloader::DDSFile::DXGIFormat::R8G8B8A8_UNorm: desc.format = Format::R8G8B8A8_UNORM; break;
					case tinyddsloader::DDSFile::DXGIFormat::R8G8B8A8_UNorm_SRGB: desc.format = Format::R8G8B8A8_UNORM_SRGB; break;
					case tinyddsloader::DDSFile::DXGIFormat::R8G8B8A8_UInt: desc.format = Format::R8G8B8A8_UINT; break;
					case tinyddsloader::DDSFile::DXGIFormat::R8G8B8A8_SNorm: desc.format = Format::R8G8B8A8_SNORM; break;
					case tinyddsloader::DDSFile::DXGIFormat::R8G8B8A8_SInt: desc.format = Format::R8G8B8A8_SINT; break;
					case tinyddsloader::DDSFile::DXGIFormat::R16G16_Float: desc.format = Format::R16G16_FLOAT; break;
					case tinyddsloader::DDSFile::DXGIFormat::R16G16_UNorm: desc.format = Format::R16G16_UNORM; break;
					case tinyddsloader::DDSFile::DXGIFormat::R16G16_UInt: desc.format = Format::R16G16_UINT; break;
					case tinyddsloader::DDSFile::DXGIFormat::R16G16_SNorm: desc.format = Format::R16G16_SNORM; break;
					case tinyddsloader::DDSFile::DXGIFormat::R16G16_SInt: desc.format = Format::R16G16_SINT; break;
					case tinyddsloader::DDSFile::DXGIFormat::D32_Float: desc.format = Format::D32_FLOAT; break;
					case tinyddsloader::DDSFile::DXGIFormat::R32_Float: desc.format = Format::R32_FLOAT; break;
					case tinyddsloader::DDSFile::DXGIFormat::R32_UInt: desc.format = Format::R32_UINT; break;
					case tinyddsloader::DDSFile::DXGIFormat::R32_SInt: desc.format = Format::R32_SINT; break;
					case tinyddsloader::DDSFile::DXGIFormat::R8G8_UNorm: desc.format = Format::R8G8_UNORM; break;
					case tinyddsloader::DDSFile::DXGIFormat::R8G8_UInt: desc.format = Format::R8G8_UINT; break;
					case tinyddsloader::DDSFile::DXGIFormat::R8G8_SNorm: desc.format = Format::R8G8_SNORM; break;
					case tinyddsloader::DDSFile::DXGIFormat::R8G8_SInt: desc.format = Format::R8G8_SINT; break;
					case tinyddsloader::DDSFile::DXGIFormat::R16_Float: desc.format = Format::R16_FLOAT; break;
					case tinyddsloader::DDSFile::DXGIFormat::D16_UNorm: desc.format = Format::D16_UNORM; break;
					case tinyddsloader::DDSFile::DXGIFormat::R16_UNorm: desc.format = Format::R16_UNORM; break;
					case tinyddsloader::DDSFile::DXGIFormat::R16_UInt: desc.format = Format::R16_UINT; break;
					case tinyddsloader::DDSFile::DXGIFormat::R16_SNorm: desc.format = Format::R16_SNORM; break;
					case tinyddsloader::DDSFile::DXGIFormat::R16_SInt: desc.format = Format::R16_SINT; break;
					case tinyddsloader::DDSFile::DXGIFormat::R8_UNorm: desc.format = Format::R8_UNORM; break;
					case tinyddsloader::DDSFile::DXGIFormat::R8_UInt: desc.format = Format::R8_UINT; break;
					case tinyddsloader::DDSFile::DXGIFormat::R8_SNorm: desc.format = Format::R8_SNORM; break;
					case tinyddsloader::DDSFile::DXGIFormat::R8_SInt: desc.format = Format::R8_SINT; break;
					case tinyddsloader::DDSFile::DXGIFormat::BC1_UNorm: desc.format = Format::BC1_UNORM; break;
					case tinyddsloader::DDSFile::DXGIFormat::BC1_UNorm_SRGB: desc.format = Format::BC1_UNORM_SRGB; break;
					case tinyddsloader::DDSFile::DXGIFormat::BC2_UNorm: desc.format = Format::BC2_UNORM; break;
					case tinyddsloader::DDSFile::DXGIFormat::BC2_UNorm_SRGB: desc.format = Format::BC2_UNORM_SRGB; break;
					case tinyddsloader::DDSFile::DXGIFormat::BC3_UNorm: desc.format = Format::BC3_UNORM; break;
					case tinyddsloader::DDSFile::DXGIFormat::BC3_UNorm_SRGB: desc.format = Format::BC3_UNORM_SRGB; break;
					case tinyddsloader::DDSFile::DXGIFormat::BC4_UNorm: desc.format = Format::BC4_UNORM; break;
					case tinyddsloader::DDSFile::DXGIFormat::BC4_SNorm: desc.format = Format::BC4_SNORM; break;
					case tinyddsloader::DDSFile::DXGIFormat::BC5_UNorm: desc.format = Format::BC5_UNORM; break;
					case tinyddsloader::DDSFile::DXGIFormat::BC5_SNorm: desc.format = Format::BC5_SNORM; break;
					case tinyddsloader::DDSFile::DXGIFormat::BC7_UNorm: desc.format = Format::BC7_UNORM; break;
					case tinyddsloader::DDSFile::DXGIFormat::BC7_UNorm_SRGB: desc.format = Format::BC7_UNORM_SRGB; break;
					default:
						assert(0); // incoming format is not supported 
						break;
					}

					std::vector<SubresourceData> InitData;
					for (uint32_t arrayIndex = 0; arrayIndex < desc.array_size; ++arrayIndex)
					{
						for (uint32_t mip = 0; mip < desc.mip_levels; ++mip)
						{
							auto imageData = dds.GetImageData(mip, arrayIndex);
							SubresourceData subresourceData;
							subresourceData.data_ptr = imageData->m_mem;
							subresourceData.row_pitch = imageData->m_memPitch;
							subresourceData.slice_pitch = imageData->m_memSlicePitch;
							InitData.push_back(subresourceData);
						}
					}

					auto dim = dds.GetTextureDimension();
					switch (dim)
					{
					case tinyddsloader::DDSFile::TextureDimension::Texture1D:
					{
						desc.type = TextureDesc::Type::TEXTURE_1D;
					}
					break;
					case tinyddsloader::DDSFile::TextureDimension::Texture2D:
					{
						desc.type = TextureDesc::Type::TEXTURE_2D;
					}
					break;
					case tinyddsloader::DDSFile::TextureDimension::Texture3D:
					{
						desc.type = TextureDesc::Type::TEXTURE_3D;
					}
					break;
					default:
						assert(0);
						break;
					}

					if (IsFormatBlockCompressed(desc.format))
					{
						desc.width = std::max(GetFormatBlockSize(desc.format), desc.width);
						desc.height = std::max(GetFormatBlockSize(desc.format), desc.height);
					}

					success = device->CreateTexture(&desc, InitData.data(), &resource->texture);
					device->SetName(&resource->texture, name.c_str());
				}
				else assert(0); // failed to load DDS

			}
			else
			{
				// png, tga, jpg, etc. loader:

				const int channelCount = 4;
				int width, height, bpp;
				unsigned char* rgb = stbi_load_from_memory(filedata, (int)filesize, &width, &height, &bpp, channelCount);

				if (rgb != nullptr)
				{
					TextureDesc desc;
					desc.height = uint32_t(height);
					desc.width = uint32_t(width);
					desc.layout = ResourceState::SHADER_RESOURCE;

					if (flags & IMPORT_COLORGRADINGLUT)
					{
						if (desc.type != TextureDesc::Type::TEXTURE_2D ||
							desc.width != 256 ||
							desc.height != 16)
						{
							wiHelper::messageBox("The Dimensions must be 256 x 16 for color grading LUT!", "Error");
						}
						else
						{
							uint32_t data[16 * 16 * 16];
							int pixel = 0;
							for (int z = 0; z < 16; ++z)
							{
								for (int y = 0; y < 16; ++y)
								{
									for (int x = 0; x < 16; ++x)
									{
										int coord = x + y * 256 + z * 16;
										data[pixel++] = ((uint32_t*)rgb)[coord];
									}
								}
							}

							desc.type = TextureDesc::Type::TEXTURE_3D;
							desc.width = 16;
							desc.height = 16;
							desc.depth = 16;
							desc.format = Format::R8G8B8A8_UNORM;
							desc.bind_flags = BindFlag::SHADER_RESOURCE;
							SubresourceData InitData;
							InitData.data_ptr = data;
							InitData.row_pitch = 16 * sizeof(uint32_t);
							InitData.slice_pitch = 16 * InitData.row_pitch;
							success = device->CreateTexture(&desc, &InitData, &resource->texture);
							device->SetName(&resource->texture, name.c_str());
						}
					}
					else
					{
						desc.bind_flags = BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
						desc.format = Format::R8G8B8A8_UNORM;
						desc.mip_levels = (uint32_t)log2(std::max(width, height)) + 1;
						desc.usage = Usage::DEFAULT;
						desc.layout = ResourceState::SHADER_RESOURCE;

						uint32_t mipwidth = width;
						std::vector<SubresourceData> InitData(desc.mip_levels);
						for (uint32_t mip = 0; mip < desc.mip_levels; ++mip)
						{
							InitData[mip].data_ptr = rgb; // attention! we don't fill the mips here correctly, just always point to the mip0 data by default. Mip levels will be created using compute shader when needed!
							InitData[mip].row_pitch = static_cast<uint32_t>(mipwidth * channelCount);
							mipwidth = std::max(1u, mipwidth / 2);
						}

						success = device->CreateTexture(&desc, InitData.data(), &resource->texture);
						device->SetName(&resource->texture, name.c_str());

						for (uint32_t i = 0; i < resource->texture.desc.mip_levels; ++i)
						{
							int subresource_index;
							subresource_index = device->CreateSubresource(&resource->texture, SubresourceType::SRV, 0, 1, i, 1);
							assert(subresource_index == i);
							subresource_index = device->CreateSubresource(&resource->texture, SubresourceType::UAV, 0, 1, i, 1);
							assert(subresource_index == i);
						}
					}
				}
				stbi_image_free(rgb);
			}
		}
		break;
		case wiResource::SOUND:
		{
			success = wiAudio::CreateSound(filedata, filesize, &resource->sound);
		}
		break;
		};

		if (success)
		{
			resource->type = type;
			resource->flags = flags;

			if (resource->filedata.empty() && (flags & IMPORT_RETAIN_FILEDATA))
			{
				// resource was loaded with external filedata, and we want to retain filedata
				resource->filedata.resize(filesize);
				std::memcpy(resource->filedata.data(), filedata, filesize);
			}
			else if (!resource->filedata.empty() && (flags & IMPORT_RETAIN_FILEDATA) == 0)
			{
				// resource was loaded using file name, and we want to discard filedata
				resource->filedata.clear();
			}

			if (type == wiResource::IMAGE && resource->texture.desc.mip_levels > 1
				&& has_flag(resource->texture.desc.bind_flags, BindFlag::UNORDERED_ACCESS))
			{
				wiRenderer::AddDeferredMIPGen(resource, true);
			}

			return resource;
		}

		return nullptr;
	}

	bool Contains(const std::string& name)
	{
		bool result = false;
		locker.lock();
		auto it = resources.find(name);
		if (it != resources.end())
		{
			auto resource = it->second.lock();
			result = resource != nullptr && resource->type != wiResource::EMPTY;
		}
		locker.unlock();
		return result;
	}

	void Clear()
	{
		locker.lock();
		resources.clear();
		locker.unlock();
	}


	void Serialize(wiArchive& archive, ResourceSerializer& seri)
	{
		if (archive.IsReadMode())
		{
			size_t serializable_count = 0;
			archive >> serializable_count;

			struct TempResource
			{
				std::string name;
				uint32_t flags = 0;
				std::vector<uint8_t> filedata;
			};
			std::vector<TempResource> temp_resources;
			temp_resources.resize(serializable_count);

			wiJobSystem::context ctx;
			std::mutex seri_locker;
			for (size_t i = 0; i < serializable_count; ++i)
			{
				auto& resource = temp_resources[i];

				archive >> resource.name;
				archive >> resource.flags;
				archive >> resource.filedata;

				resource.name = archive.GetSourceDirectory() + resource.name;

				// "Loading" the resource can happen asynchronously to serialization of file data, to improve performance
				wiJobSystem::Execute(ctx, [i, &temp_resources, &seri_locker, &seri](wiJobArgs args) {
					auto& tmp_resource = temp_resources[i];
					auto res = Load(tmp_resource.name, tmp_resource.flags, tmp_resource.filedata.data(), tmp_resource.filedata.size());
					seri_locker.lock();
					seri.resources.push_back(res);
					seri_locker.unlock();
				});
			}

			wiJobSystem::Wait(ctx);
		}
		else
		{
			locker.lock();
			size_t serializable_count = 0;

			if (mode == MODE_ALLOW_RETAIN_FILEDATA_BUT_DISABLE_EMBEDDING)
			{
				// Simply not serialize any embedded resources
				serializable_count = 0;
				archive << serializable_count;
			}
			else
			{
				// Count embedded resources:
				for (auto& it : resources)
				{
					std::shared_ptr<wiResource> resource = it.second.lock();
					if (resource != nullptr && !resource->filedata.empty())
					{
						serializable_count++;
					}
				}

				// Write all embedded resources:
				archive << serializable_count;
				for (auto& it : resources)
				{
					std::shared_ptr<wiResource> resource = it.second.lock();

					if (resource != nullptr && !resource->filedata.empty())
					{
						std::string name = it.first;
						wiHelper::MakePathRelative(archive.GetSourceDirectory(), name);

						archive << name;
						archive << resource->flags;
						archive << resource->filedata;
					}
				}
			}
			locker.unlock();
		}
	}

}
