#include "wiResourceManager.h"
#include "wiRenderer.h"
#include "wiHelper.h"
#include "wiTextureHelper.h"
#include "wiUnorderedMap.h"
#include "wiBacklog.h"

#include "Utility/qoi.h"
#include "Utility/stb_image.h"
#include "Utility/tinyddsloader.h"
#include "Utility/basis_universal/transcoder/basisu_transcoder.h"

#include <algorithm>
#include <mutex>
#include <unordered_map>

using namespace wi::graphics;

namespace wi
{
	struct ResourceInternal
	{
		resourcemanager::Flags flags = resourcemanager::Flags::NONE;
		wi::graphics::Texture texture;
		int srgb_subresource = -1;
		wi::audio::Sound sound;
		std::string script;
		wi::video::Video video;
		wi::vector<uint8_t> filedata;
		int font_style = -1;
	};

	const wi::vector<uint8_t>& Resource::GetFileData() const
	{
		const ResourceInternal* resourceinternal = (ResourceInternal*)internal_state.get();
		return resourceinternal->filedata;
	}
	const wi::graphics::Texture& Resource::GetTexture() const
	{
		const ResourceInternal* resourceinternal = (ResourceInternal*)internal_state.get();
		return resourceinternal->texture;
	}
	const wi::audio::Sound& Resource::GetSound() const
	{
		const ResourceInternal* resourceinternal = (ResourceInternal*)internal_state.get();
		return resourceinternal->sound;
	}
	const std::string& Resource::GetScript() const
	{
		const ResourceInternal* resourceinternal = (ResourceInternal*)internal_state.get();
		return resourceinternal->script;
	}
	const wi::video::Video& Resource::GetVideo() const
	{
		const ResourceInternal* resourceinternal = (ResourceInternal*)internal_state.get();
		return resourceinternal->video;
	}
	int Resource::GetTextureSRGBSubresource() const
	{
		const ResourceInternal* resourceinternal = (ResourceInternal*)internal_state.get();
		return resourceinternal->srgb_subresource;
	}
	int Resource::GetFontStyle() const
	{
		const ResourceInternal* resourceinternal = (ResourceInternal*)internal_state.get();
		return resourceinternal->font_style;
	}

	void Resource::SetFileData(const wi::vector<uint8_t>& data)
	{
		if (internal_state == nullptr)
		{
			internal_state = std::make_shared<ResourceInternal>();
		}
		ResourceInternal* resourceinternal = (ResourceInternal*)internal_state.get();
		resourceinternal->filedata = data;
	}
	void Resource::SetFileData(wi::vector<uint8_t>&& data)
	{
		if (internal_state == nullptr)
		{
			internal_state = std::make_shared<ResourceInternal>();
		}
		ResourceInternal* resourceinternal = (ResourceInternal*)internal_state.get();
		resourceinternal->filedata = data;
	}
	void Resource::SetTexture(const wi::graphics::Texture& texture, int srgb_subresource)
	{
		if (internal_state == nullptr)
		{
			internal_state = std::make_shared<ResourceInternal>();
		}
		ResourceInternal* resourceinternal = (ResourceInternal*)internal_state.get();
		resourceinternal->texture = texture;
		resourceinternal->srgb_subresource = srgb_subresource;
	}
	void Resource::SetSound(const wi::audio::Sound& sound)
	{
		if (internal_state == nullptr)
		{
			internal_state = std::make_shared<ResourceInternal>();
		}
		ResourceInternal* resourceinternal = (ResourceInternal*)internal_state.get();
		resourceinternal->sound = sound;
	}
	void Resource::SetScript(const std::string& script)
	{
		if (internal_state == nullptr)
		{
			internal_state = std::make_shared<ResourceInternal>();
		}
		ResourceInternal* resourceinternal = (ResourceInternal*)internal_state.get();
		resourceinternal->script = script;
	}
	void Resource::SetVideo(const wi::video::Video& video)
	{
		if (internal_state == nullptr)
		{
			internal_state = std::make_shared<ResourceInternal>();
		}
		ResourceInternal* resourceinternal = (ResourceInternal*)internal_state.get();
		resourceinternal->video = video;
	}

	void Resource::SetOutdated()
	{
		if (internal_state == nullptr)
		{
			internal_state = std::make_shared<ResourceInternal>();
		}
		ResourceInternal* resourceinternal = (ResourceInternal*)internal_state.get();
		resourceinternal->flags |= resourcemanager::Flags::IMPORT_DELAY; // this will cause resource to be recreated, but let using old file data like delayed loading
	}

	namespace resourcemanager
	{
		static std::mutex locker;
		static std::unordered_map<std::string, std::weak_ptr<ResourceInternal>> resources;
		static Mode mode = Mode::DISCARD_FILEDATA_AFTER_LOAD;

		void SetMode(Mode param)
		{
			mode = param;
		}
		Mode GetMode()
		{
			return mode;
		}

		enum class DataType
		{
			IMAGE,
			SOUND,
			SCRIPT,
			VIDEO,
			FONTSTYLE,
		};
		static const wi::unordered_map<std::string, DataType> types = {
			{"BASIS", DataType::IMAGE},
			{"KTX2", DataType::IMAGE},
			{"JPG", DataType::IMAGE},
			{"JPEG", DataType::IMAGE},
			{"PNG", DataType::IMAGE},
			{"BMP", DataType::IMAGE},
			{"DDS", DataType::IMAGE},
			{"TGA", DataType::IMAGE},
			{"QOI", DataType::IMAGE},
			{"HDR", DataType::IMAGE},
			{"WAV", DataType::SOUND},
			{"OGG", DataType::SOUND},
			{"LUA", DataType::SCRIPT},
			{"MP4", DataType::VIDEO},
			{"TTF", DataType::FONTSTYLE},
		};
		wi::vector<std::string> GetSupportedImageExtensions()
		{
			wi::vector<std::string> ret;
			for (auto& x : types)
			{
				if (x.second == DataType::IMAGE)
				{
					ret.push_back(x.first);
				}
			}
			return ret;
		}
		wi::vector<std::string> GetSupportedSoundExtensions()
		{
			wi::vector<std::string> ret;
			for (auto& x : types)
			{
				if (x.second == DataType::SOUND)
				{
					ret.push_back(x.first);
				}
			}
			return ret;
		}
		wi::vector<std::string> GetSupportedVideoExtensions()
		{
			wi::vector<std::string> ret;
			for (auto& x : types)
			{
				if (x.second == DataType::VIDEO)
				{
					ret.push_back(x.first);
				}
			}
			return ret;
		}
		wi::vector<std::string> GetSupportedScriptExtensions()
		{
			wi::vector<std::string> ret;
			for (auto& x : types)
			{
				if (x.second == DataType::SCRIPT)
				{
					ret.push_back(x.first);
				}
			}
			return ret;
		}
		wi::vector<std::string> GetSupportedFontStyleExtensions()
		{
			wi::vector<std::string> ret;
			for (auto& x : types)
			{
				if (x.second == DataType::FONTSTYLE)
				{
					ret.push_back(x.first);
				}
			}
			return ret;
		}

		Resource Load(const std::string& name, Flags flags, const uint8_t* filedata, size_t filesize)
		{
			if (mode == Mode::DISCARD_FILEDATA_AFTER_LOAD)
			{
				flags &= ~Flags::IMPORT_RETAIN_FILEDATA;
			}

			locker.lock();
			std::weak_ptr<ResourceInternal>& weak_resource = resources[name];
			std::shared_ptr<ResourceInternal> resource = weak_resource.lock();

			static bool basis_init = false; // within lock!
			if (!basis_init)
			{
				basis_init = true;
				basist::basisu_transcoder_init();
			}

			if (resource == nullptr)
			{
				resource = std::make_shared<ResourceInternal>();
				resources[name] = resource;
			}
			else
			{
				if (!has_flag(flags, Flags::IMPORT_DELAY) && has_flag(resource->flags, Flags::IMPORT_DELAY))
				{
					// If this is not an IMPORT_DELAY load, but this resource load was incomplete, using IMPORT_DELAY,
					//	then continue loading it as normal from existing file data and remove IMPORT_DELAY flag from it
					resource->flags &= ~Flags::IMPORT_DELAY;
				}
				else
				{
					Resource retVal;
					retVal.internal_state = resource;
					locker.unlock();
					return retVal;
				}
			}
			locker.unlock();

			if (filedata == nullptr || filesize == 0)
			{
				if (resource->filedata.empty() && !wi::helper::FileRead(name, resource->filedata))
				{
					resource.reset();
					return Resource();
				}
				filedata = resource->filedata.data();
				filesize = resource->filedata.size();
			}

			bool success = false;

			if (has_flag(flags, Flags::IMPORT_DELAY))
			{
				success = true;
			}
			else
			{
				std::string ext = wi::helper::toUpper(wi::helper::GetExtensionFromFileName(name));
				DataType type;

				// dynamic type selection:
				{
					auto it = types.find(ext);
					if (it != types.end())
					{
						type = it->second;
					}
					else
					{
						return Resource();
					}
				}

				switch (type)
				{
				case DataType::IMAGE:
				{
					GraphicsDevice* device = wi::graphics::GetDevice();
					if (!ext.compare("KTX2"))
					{
						basist::ktx2_transcoder transcoder;
						if (transcoder.init(filedata, (uint32_t)filesize))
						{
							TextureDesc desc;
							desc.bind_flags = BindFlag::SHADER_RESOURCE;
							desc.width = transcoder.get_width();
							desc.height = transcoder.get_height();
							desc.array_size = std::max(desc.array_size, transcoder.get_layers() * transcoder.get_faces());
							desc.mip_levels = transcoder.get_levels();
							desc.misc_flags = ResourceMiscFlag::TYPED_FORMAT_CASTING;
							if (transcoder.get_faces() == 6)
							{
								desc.misc_flags |= ResourceMiscFlag::TEXTURECUBE;
							}

							basist::transcoder_texture_format fmt = basist::transcoder_texture_format::cTFRGBA32;
							desc.format = Format::R8G8B8A8_UNORM;

							bool import_compressed = has_flag(flags, Flags::IMPORT_BLOCK_COMPRESSED);
							if (import_compressed)
							{
								// BC5 is disabled because it's missing green channel!
								//if (has_flag(flags, Flags::IMPORT_NORMALMAP))
								//{
								//	fmt = basist::transcoder_texture_format::cTFBC5_RG;
								//	desc.format = Format::BC5_UNORM;
								//	desc.swizzle.r = ComponentSwizzle::R;
								//	desc.swizzle.g = ComponentSwizzle::G;
								//	desc.swizzle.b = ComponentSwizzle::ONE;
								//	desc.swizzle.a = ComponentSwizzle::ONE;
								//}
								//else
								{
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
								}
							}
							uint32_t bytes_per_block = basis_get_bytes_per_block_or_pixel(fmt);

							if (transcoder.start_transcoding())
							{
								// all subresources will use one allocation for transcoder destination, so compute combined size:
								size_t transcoded_data_size = 0;
								const uint32_t layers = std::max(1u, transcoder.get_layers());
								const uint32_t faces = transcoder.get_faces();
								const uint32_t levels = transcoder.get_levels();
								for (uint32_t layer = 0; layer < layers; ++layer)
								{
									for (uint32_t face = 0; face < faces; ++face)
									{
										for (uint32_t mip = 0; mip < levels; ++mip)
										{
											basist::ktx2_image_level_info level_info;
											if (transcoder.get_image_level_info(level_info, mip, layer, face))
											{
												uint32_t pixel_or_block_count = (import_compressed
													? level_info.m_total_blocks
													: (level_info.m_orig_width * level_info.m_orig_height));
												transcoded_data_size += bytes_per_block * pixel_or_block_count;
											}
										}
									}
								}
								wi::vector<uint8_t> transcoded_data(transcoded_data_size);
								
								wi::vector<SubresourceData> InitData;
								size_t transcoded_data_offset = 0;
								for (uint32_t layer = 0; layer < layers; ++layer)
								{
									for (uint32_t face = 0; face < faces; ++face)
									{
										for (uint32_t mip = 0; mip < levels; ++mip)
										{
											basist::ktx2_image_level_info level_info;
											if (transcoder.get_image_level_info(level_info, mip, layer, face))
											{
												void* data_ptr = transcoded_data.data() + transcoded_data_offset;
												uint32_t pixel_or_block_count = (import_compressed
													? level_info.m_total_blocks
													: (level_info.m_orig_width * level_info.m_orig_height));
												transcoded_data_offset += bytes_per_block * pixel_or_block_count;
												if (transcoder.transcode_image_level(
													mip,
													layer,
													face,
													data_ptr,
													pixel_or_block_count,
													fmt
												))
												{
													SubresourceData subresourceData;
													subresourceData.data_ptr = data_ptr;
													subresourceData.row_pitch = (import_compressed ? level_info.m_num_blocks_x : level_info.m_orig_width) * bytes_per_block;
													subresourceData.slice_pitch = subresourceData.row_pitch * (import_compressed ? level_info.m_num_blocks_y : level_info.m_orig_height);
													InitData.push_back(subresourceData);
												}
												else
												{
													wi::backlog::post("KTX2 transcoding error while loading image!", wi::backlog::LogLevel::Error);
													assert(0);
												}
											}
											else
											{
												wi::backlog::post("KTX2 transcoding error while loading image level info!", wi::backlog::LogLevel::Error);
												assert(0);
											}
										}
									}
								}

								if (!InitData.empty())
								{
									success = device->CreateTexture(&desc, InitData.data(), &resource->texture);
									device->SetName(&resource->texture, name.c_str());

									Format srgb_format = GetFormatSRGB(desc.format);
									if (srgb_format != Format::UNKNOWN && srgb_format != desc.format)
									{
										resource->srgb_subresource = device->CreateSubresource(
											&resource->texture,
											SubresourceType::SRV,
											0, -1,
											0, -1,
											&srgb_format
										);
									}
								}
							}
							transcoder.clear();
						}
					}
					else if (!ext.compare("BASIS"))
					{
						basist::basisu_transcoder transcoder;
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
									desc.misc_flags = ResourceMiscFlag::TYPED_FORMAT_CASTING;

									basist::transcoder_texture_format fmt = basist::transcoder_texture_format::cTFRGBA32;
									desc.format = Format::R8G8B8A8_UNORM;

									bool import_compressed = has_flag(flags, Flags::IMPORT_BLOCK_COMPRESSED);
									if (import_compressed)
									{
										// BC5 is disabled because it's missing green channel!
										//if (has_flag(flags, Flags::IMPORT_NORMALMAP))
										//{
										//	fmt = basist::transcoder_texture_format::cTFBC5_RG;
										//	desc.format = Format::BC5_UNORM;
										//	desc.swizzle.r = ComponentSwizzle::R;
										//	desc.swizzle.g = ComponentSwizzle::G;
										//	desc.swizzle.b = ComponentSwizzle::ONE;
										//	desc.swizzle.a = ComponentSwizzle::ONE;
										//}
										//else
										{
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
										}
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
												uint32_t pixel_or_block_count = (import_compressed
													? level_info.m_total_blocks
													: (level_info.m_orig_width * level_info.m_orig_height));
												transcoded_data_size += bytes_per_block * pixel_or_block_count;
											}
										}
										wi::vector<uint8_t> transcoded_data(transcoded_data_size);

										wi::vector<SubresourceData> InitData;
										size_t transcoded_data_offset = 0;
										for (uint32_t mip = 0; mip < desc.mip_levels; ++mip)
										{
											basist::basisu_image_level_info level_info;
											if (transcoder.get_image_level_info(filedata, (uint32_t)filesize, level_info, 0, mip))
											{
												void* data_ptr = transcoded_data.data() + transcoded_data_offset;
												uint32_t pixel_or_block_count = (import_compressed
													? level_info.m_total_blocks
													: (level_info.m_orig_width * level_info.m_orig_height));
												transcoded_data_offset += pixel_or_block_count * bytes_per_block;
												if (transcoder.transcode_image_level(
													filedata,
													(uint32_t)filesize,
													image_index,
													mip,
													data_ptr,
													pixel_or_block_count,
													fmt
												))
												{
													SubresourceData subresourceData;
													subresourceData.data_ptr = data_ptr;
													subresourceData.row_pitch = (import_compressed ? level_info.m_num_blocks_x : level_info.m_orig_width) * bytes_per_block;
													subresourceData.slice_pitch = subresourceData.row_pitch * (import_compressed ? level_info.m_num_blocks_y : level_info.m_orig_height);
													InitData.push_back(subresourceData);
												}
												else
												{
													wi::backlog::post("BASIS transcoding error while loading image!", wi::backlog::LogLevel::Error);
													assert(0);
												}
											}
											else
											{
												wi::backlog::post("BASIS transcoding error while loading image level info!", wi::backlog::LogLevel::Error);
												assert(0);
											}
										}

										if (!InitData.empty())
										{
											success = device->CreateTexture(&desc, InitData.data(), &resource->texture);
											device->SetName(&resource->texture, name.c_str());

											Format srgb_format = GetFormatSRGB(desc.format);
											if (srgb_format != Format::UNKNOWN && srgb_format != desc.format)
											{
												resource->srgb_subresource = device->CreateSubresource(
													&resource->texture,
													SubresourceType::SRV,
													0, -1,
													0, -1,
													&srgb_format
												);
											}
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
							desc.misc_flags = ResourceMiscFlag::TYPED_FORMAT_CASTING;

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
							case tinyddsloader::DDSFile::DXGIFormat::R9G9B9E5_SHAREDEXP: desc.format = Format::R9G9B9E5_SHAREDEXP; break;
							case tinyddsloader::DDSFile::DXGIFormat::B8G8R8X8_UNorm: desc.format = Format::B8G8R8A8_UNORM; break;
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
							case tinyddsloader::DDSFile::DXGIFormat::BC6H_SF16: desc.format = Format::BC6H_SF16; break;
							case tinyddsloader::DDSFile::DXGIFormat::BC6H_UF16: desc.format = Format::BC6H_UF16; break;
							case tinyddsloader::DDSFile::DXGIFormat::BC7_UNorm: desc.format = Format::BC7_UNORM; break;
							case tinyddsloader::DDSFile::DXGIFormat::BC7_UNorm_SRGB: desc.format = Format::BC7_UNORM_SRGB; break;
							default:
								assert(0); // incoming format is not supported 
								break;
							}

							if (desc.format == Format::BC5_UNORM)
							{
								desc.swizzle.r = ComponentSwizzle::R;
								desc.swizzle.g = ComponentSwizzle::G;
								desc.swizzle.b = ComponentSwizzle::ONE;
								desc.swizzle.a = ComponentSwizzle::ONE;
							}

							wi::vector<SubresourceData> InitData;
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
								desc.width = AlignTo(desc.width, GetFormatBlockSize(desc.format));
								desc.height = AlignTo(desc.height, GetFormatBlockSize(desc.format));
							}

							success = device->CreateTexture(&desc, InitData.data(), &resource->texture);
							device->SetName(&resource->texture, name.c_str());

							Format srgb_format = GetFormatSRGB(desc.format);
							if (srgb_format != Format::UNKNOWN && srgb_format != desc.format)
							{
								resource->srgb_subresource = device->CreateSubresource(
									&resource->texture,
									SubresourceType::SRV,
									0, -1,
									0, -1,
									&srgb_format
								);
							}
						}
						else assert(0); // failed to load DDS

					}
					else if (!ext.compare("HDR"))
					{
						int height, width, channels; // stb_image
						float* data = stbi_loadf_from_memory(filedata, (int)filesize, &width, &height, &channels, 0);
						static constexpr bool allow_packing = true; // we now always assume that we won't need full precision float textures, so pack them for memory saving

						if (data != nullptr)
						{
							TextureDesc desc;
							desc.width = (uint32_t)width;
							desc.height = (uint32_t)height;
							switch (channels)
							{
							default:
							case 4:
								if (allow_packing)
								{
									desc.format = Format::R16G16B16A16_FLOAT;
									const XMFLOAT4* data_full = (const XMFLOAT4*)data;
									XMHALF4* data_packed = (XMHALF4*)data;
									for (int i = 0; i < width * height; ++i)
									{
										XMStoreHalf4(data_packed + i, XMLoadFloat4(data_full + i));
									}
								}
								else
								{
									desc.format = Format::R32G32B32A32_FLOAT;
								}
								break;
							case 3:
								if (allow_packing)
								{
									desc.format = Format::R9G9B9E5_SHAREDEXP;
									const XMFLOAT3* data_full = (const XMFLOAT3*)data;
									XMFLOAT3SE* data_packed = (XMFLOAT3SE*)data;
									for (int i = 0; i < width * height; ++i)
									{
										XMStoreFloat3SE(data_packed + i, XMLoadFloat3(data_full + i));
									}
								}
								else
								{
									desc.format = Format::R32G32B32_FLOAT;
								}
								break;
							case 2:
								if (allow_packing)
								{
									desc.format = Format::R16G16_FLOAT;
									const XMFLOAT2* data_full = (const XMFLOAT2*)data;
									XMHALF2* data_packed = (XMHALF2*)data;
									for (int i = 0; i < width * height; ++i)
									{
										XMStoreHalf2(data_packed + i, XMLoadFloat2(data_full + i));
									}
								}
								else
								{
									desc.format = Format::R32G32_FLOAT;
								}
								break;
							case 1:
								if (allow_packing)
								{
									desc.format = Format::R16_FLOAT;
									HALF* data_packed = (HALF*)data;
									for (int i = 0; i < width * height; ++i)
									{
										data_packed[i] = XMConvertFloatToHalf(data[i]);
									}
								}
								else
								{
									desc.format = Format::R32_FLOAT;
								}
								break;
							}
							desc.bind_flags = BindFlag::SHADER_RESOURCE;
							desc.mip_levels = 1;
							SubresourceData InitData;
							InitData.data_ptr = data;
							InitData.row_pitch = width * GetFormatStride(desc.format);
							success = device->CreateTexture(&desc, &InitData, &resource->texture);
							device->SetName(&resource->texture, name.c_str());

							stbi_image_free(data);
						}
					}
					else
					{
						// qoi, png, tga, jpg, etc. loader:
						int height = 0, width = 0, channels = 0;
						bool is_16bit = false;
						Format format = Format::R8G8B8A8_UNORM;
						Format bc_format = Format::BC3_UNORM;
						Swizzle swizzle = { ComponentSwizzle::R, ComponentSwizzle::G, ComponentSwizzle::B, ComponentSwizzle::A };

						void* rgba;
						if (!ext.compare("QOI"))
						{
							qoi_desc desc = {};
							rgba = qoi_decode(filedata, (int)filesize, &desc, 4);
							// redefine width, height to avoid further conditionals
							height = desc.height;
							width = desc.width;
							channels = 4;
						}
						else
						{
							if (!has_flag(flags, Flags::IMPORT_COLORGRADINGLUT) && stbi_is_16_bit_from_memory(filedata, (int)filesize))
							{
								is_16bit = true;
								rgba = stbi_load_16_from_memory(filedata, (int)filesize, &width, &height, &channels, 0);
								switch (channels)
								{
								case 1:
									format = Format::R16_UNORM;
									bc_format = Format::BC4_UNORM;
									swizzle = { ComponentSwizzle::R, ComponentSwizzle::R, ComponentSwizzle::R, ComponentSwizzle::ONE };
									break;
								case 2:
									format = Format::R16G16_UNORM;
									bc_format = Format::BC5_UNORM;
									swizzle = { ComponentSwizzle::R, ComponentSwizzle::G, ComponentSwizzle::ONE, ComponentSwizzle::ONE };
									break;
								case 3:
								{
									// Graphics API doesn't support 3 channel formats, so need to expand to RGBA:
									struct Color3
									{
										uint16_t r, g, b;
									};
									const Color3* color3 = (const Color3*)rgba;
									wi::Color16* color4 = (wi::Color16*)malloc(width * height * sizeof(wi::Color16));
									for (int i = 0; i < width * height; ++i)
									{
										color4[i].setR(color3[i].r);
										color4[i].setG(color3[i].g);
										color4[i].setB(color3[i].b);
										color4[i].setA(65535);
									}
									free(rgba);
									rgba = color4;
									format = Format::R16G16B16A16_UNORM;
									bc_format = Format::BC1_UNORM;
									swizzle = { ComponentSwizzle::R, ComponentSwizzle::G, ComponentSwizzle::B, ComponentSwizzle::ONE };
								}
								break;
								case 4:
								default:
									format = Format::R16G16B16A16_UNORM;
									bc_format = Format::BC3_UNORM;
									swizzle = { ComponentSwizzle::R, ComponentSwizzle::G, ComponentSwizzle::B, ComponentSwizzle::A };
									break;
								}
							}
							else
							{
								rgba = stbi_load_from_memory(filedata, (int)filesize, &width, &height, &channels, 0);
								switch (channels)
								{
								case 1:
									format = Format::R8_UNORM;
									bc_format = Format::BC4_UNORM;
									swizzle = { ComponentSwizzle::R, ComponentSwizzle::R, ComponentSwizzle::R, ComponentSwizzle::ONE };
									break;
								case 2:
									format = Format::R8G8_UNORM;
									bc_format = Format::BC5_UNORM;
									swizzle = { ComponentSwizzle::R, ComponentSwizzle::G, ComponentSwizzle::ONE, ComponentSwizzle::ONE };
									break;
								case 3:
								{
									// Graphics API doesn't support 3 channel formats, so need to expand to RGBA:
									struct Color3
									{
										uint8_t r, g, b;
									};
									const Color3* color3 = (const Color3*)rgba;
									wi::Color* color4 = (wi::Color*)malloc(width * height * sizeof(wi::Color));
									for (int i = 0; i < width * height; ++i)
									{
										color4[i].setR(color3[i].r);
										color4[i].setG(color3[i].g);
										color4[i].setB(color3[i].b);
										color4[i].setA(255);
									}
									free(rgba);
									rgba = color4;
									format = Format::R8G8B8A8_UNORM;
									bc_format = Format::BC1_UNORM;
									swizzle = { ComponentSwizzle::R, ComponentSwizzle::G, ComponentSwizzle::B, ComponentSwizzle::ONE };
								}
								break;
								case 4:
								default:
									format = Format::R8G8B8A8_UNORM;
									bc_format = Format::BC3_UNORM;
									swizzle = { ComponentSwizzle::R, ComponentSwizzle::G, ComponentSwizzle::B, ComponentSwizzle::A };
									break;
								}
							}
						}

						if (rgba != nullptr)
						{
							TextureDesc desc;
							desc.height = uint32_t(height);
							desc.width = uint32_t(width);
							desc.layout = ResourceState::SHADER_RESOURCE;
							desc.format = format;
							desc.swizzle = swizzle;

							if (has_flag(flags, Flags::IMPORT_COLORGRADINGLUT))
							{
								if (desc.type != TextureDesc::Type::TEXTURE_2D ||
									desc.width != 256 ||
									desc.height != 16 ||
									format != Format::R8G8B8A8_UNORM)
								{
									wi::helper::messageBox("The Dimensions must be 256 x 16 for color grading LUT and format must be RGB or RGBA!", "Error");
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
												data[pixel++] = ((uint32_t*)rgba)[coord];
											}
										}
									}

									desc.type = TextureDesc::Type::TEXTURE_3D;
									desc.width = 16;
									desc.height = 16;
									desc.depth = 16;
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
								desc.mip_levels = GetMipCount(desc.width, desc.height);
								desc.usage = Usage::DEFAULT;
								desc.layout = ResourceState::SHADER_RESOURCE;
								desc.misc_flags = ResourceMiscFlag::TYPED_FORMAT_CASTING;

								uint32_t mipwidth = width;
								SubresourceData init_data[32]; // don't support more than 32 mips anyway
								for (uint32_t mip = 0; mip < desc.mip_levels; ++mip)
								{
									init_data[mip].data_ptr = rgba; // attention! we don't fill the mips here correctly, just always point to the mip0 data by default. Mip levels will be created using compute shader when needed!
									init_data[mip].row_pitch = uint32_t(mipwidth * GetFormatStride(desc.format));
									mipwidth = std::max(1u, mipwidth / 2);
								}

								success = device->CreateTexture(&desc, init_data, &resource->texture);
								device->SetName(&resource->texture, name.c_str());

								for (uint32_t i = 0; i < resource->texture.desc.mip_levels; ++i)
								{
									int subresource_index;
									subresource_index = device->CreateSubresource(&resource->texture, SubresourceType::SRV, 0, 1, i, 1);
									assert(subresource_index == i);
									subresource_index = device->CreateSubresource(&resource->texture, SubresourceType::UAV, 0, 1, i, 1);
									assert(subresource_index == i);
								}

								// This part must be AFTER mip level subresource creation:
								Format srgb_format = GetFormatSRGB(desc.format);
								if (srgb_format != Format::UNKNOWN && srgb_format != desc.format)
								{
									resource->srgb_subresource = device->CreateSubresource(
										&resource->texture,
										SubresourceType::SRV,
										0, -1,
										0, -1,
										&srgb_format
									);
								}

								wi::renderer::AddDeferredMIPGen(resource->texture, true);

								if (has_flag(flags, Flags::IMPORT_BLOCK_COMPRESSED))
								{
									// Schedule additional task to compress into BC format and replace resource texture:
									Texture uncompressed_src = std::move(resource->texture);
									resource->srgb_subresource = -1;

									desc.format = bc_format;

									if (has_flag(flags, Flags::IMPORT_NORMALMAP))
									{
										desc.format = Format::BC5_UNORM;
										desc.swizzle = { ComponentSwizzle::R, ComponentSwizzle::G, ComponentSwizzle::ONE, ComponentSwizzle::ONE };
									}

									desc.bind_flags = BindFlag::SHADER_RESOURCE;

									const uint32_t block_size = GetFormatBlockSize(desc.format);
									desc.width = AlignTo(desc.width, block_size);
									desc.height = AlignTo(desc.height, block_size);
									desc.mip_levels = GetMipCount(desc.width, desc.height, desc.depth, block_size, block_size);

									success = device->CreateTexture(&desc, nullptr, &resource->texture);
									device->SetName(&resource->texture, name.c_str());

									// This part must be AFTER mip level subresource creation:
									Format srgb_format = GetFormatSRGB(desc.format);
									if (srgb_format != Format::UNKNOWN && srgb_format != desc.format)
									{
										resource->srgb_subresource = device->CreateSubresource(
											&resource->texture,
											SubresourceType::SRV,
											0, -1,
											0, -1,
											&srgb_format
										);
									}

									wi::renderer::AddDeferredBlockCompression(uncompressed_src, resource->texture);
								}
							}
						}
						free(rgba);
					}
				}
				break;

				case DataType::SOUND:
				{
					success = wi::audio::CreateSound(filedata, filesize, &resource->sound);
				}
				break;

				case DataType::SCRIPT:
				{
					resource->script.resize(filesize);
					std::memcpy(resource->script.data(), filedata, filesize);
					success = true;
				}
				break;

				case DataType::VIDEO:
				{
					success = wi::video::CreateVideo(filedata, filesize, &resource->video);
				}
				break;

				case DataType::FONTSTYLE:
				{
					resource->font_style = wi::font::AddFontStyle(name, filedata, filesize, true);
					success = resource->font_style >= 0;
				}
				break;

				};
			}

			if (success)
			{
				resource->flags = flags;

				if (resource->filedata.empty() && (has_flag(flags, Flags::IMPORT_RETAIN_FILEDATA) || has_flag(flags, Flags::IMPORT_DELAY)))
				{
					// resource was loaded with external filedata, and we want to retain filedata
					//	this must also happen when using IMPORT_DELAY!
					resource->filedata.resize(filesize);
					std::memcpy(resource->filedata.data(), filedata, filesize);
				}
				else if (!resource->filedata.empty() && has_flag(flags, Flags::IMPORT_RETAIN_FILEDATA) == 0)
				{
					// resource was loaded using file name, and we want to discard filedata
					resource->filedata.clear();
				}

				Resource retVal;
				retVal.internal_state = resource;
				return retVal;
			}

			return Resource();
		}

		bool Contains(const std::string& name)
		{
			bool result = false;
			locker.lock();
			auto it = resources.find(name);
			if (it != resources.end())
			{
				auto resource = it->second.lock();
				result = resource != nullptr;
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


		void Serialize_READ(wi::Archive& archive, ResourceSerializer& seri)
		{
			assert(archive.IsReadMode());
			size_t serializable_count = 0;
			archive >> serializable_count;

			struct TempResource
			{
				std::string name;
				Flags flags = Flags::NONE;
				wi::vector<uint8_t> filedata;
			};
			wi::vector<TempResource> temp_resources;
			temp_resources.resize(serializable_count);

			wi::jobsystem::context ctx;
			std::mutex seri_locker;
			for (size_t i = 0; i < serializable_count; ++i)
			{
				auto& resource = temp_resources[i];

				archive >> resource.name;
				uint32_t flags_temp;
				archive >> flags_temp;
				resource.flags = (Flags)flags_temp;
				archive >> resource.filedata;

				resource.name = archive.GetSourceDirectory() + resource.name;
				resource.flags |= Flags::IMPORT_DELAY; // delay resource creation, to be able to receive additional flags (this way only file data is loaded)

				// "Loading" the resource can happen asynchronously to serialization of file data, to improve performance
				wi::jobsystem::Execute(ctx, [i, &temp_resources, &seri_locker, &seri](wi::jobsystem::JobArgs args) {
					auto& tmp_resource = temp_resources[i];
					auto res = Load(tmp_resource.name, tmp_resource.flags, tmp_resource.filedata.data(), tmp_resource.filedata.size());
					seri_locker.lock();
					seri.resources.push_back(res);
					seri_locker.unlock();
					});
			}

			wi::jobsystem::Wait(ctx);
		}
		void Serialize_WRITE(wi::Archive& archive, const wi::unordered_set<std::string>& resource_names)
		{
			assert(!archive.IsReadMode());

			locker.lock();
			size_t serializable_count = 0;

			if (mode == Mode::ALLOW_RETAIN_FILEDATA_BUT_DISABLE_EMBEDDING)
			{
				// Simply not serialize any embedded resources
				serializable_count = 0;
				archive << serializable_count;
			}
			else
			{
				// Count embedded resources:
				for (auto& name : resource_names)
				{
					auto it = resources.find(name);
					if (it == resources.end())
						continue;
					std::shared_ptr<ResourceInternal> resource = it->second.lock();
					if (resource != nullptr && !resource->filedata.empty())
					{
						serializable_count++;
					}
				}

				// Write all embedded resources:
				archive << serializable_count;
				for (auto& name : resource_names)
				{
					auto it = resources.find(name);
					if (it == resources.end())
						continue;
					std::shared_ptr<ResourceInternal> resource = it->second.lock();

					if (resource != nullptr && !resource->filedata.empty())
					{
						std::string name = it->first;
						wi::helper::MakePathRelative(archive.GetSourceDirectory(), name);

						archive << name;
						archive << (uint32_t)resource->flags;
						archive << resource->filedata;
					}
				}
			}
			locker.unlock();
		}

	}

}
