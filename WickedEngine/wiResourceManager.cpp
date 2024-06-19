#include "wiResourceManager.h"
#include "wiRenderer.h"
#include "wiHelper.h"
#include "wiTextureHelper.h"
#include "wiUnorderedMap.h"
#include "wiBacklog.h"
#include "wiJobSystem.h"

#include "Utility/qoi.h"
#include "Utility/stb_image.h"
#include "Utility/basis_universal/transcoder/basisu_transcoder.h"
#include "Utility/dds.h"

#include <algorithm>
#include <mutex>
#include <unordered_map>

using namespace wi::graphics;

namespace wi
{
	struct StreamingTexture
	{
		struct StreamingSubresourceData
		{
			size_t data_offset = 0;
			uint32_t row_pitch = 0;
			uint32_t slice_pitch = 0;
		};
		StreamingSubresourceData streaming_data[16] = {};
		uint32_t mip_count = 0; // mip count of full resource
		float min_lod_clamp_absolute = 0; // relative to mip_count of full resource
	};
	//static constexpr size_t streaming_texture_min_size = 4096; // 4KB is the minimum texture memory alignment
	static constexpr size_t streaming_texture_min_size = 64 * 1024; // 64KB is the usual texture memory alignment, this allows higher base tex size than 4KB

	struct ResourceInternal
	{
		resourcemanager::Flags flags = resourcemanager::Flags::NONE;
		wi::graphics::Texture texture;
		int srgb_subresource = -1;
		wi::audio::Sound sound;
		std::string script;
		size_t script_hash = 0;
		wi::video::Video video;
		wi::vector<uint8_t> filedata;
		int font_style = -1;

		// Original filename:
		std::string filename;

		// Container file is different from original filename when
		//	multiple resources are embedded inside one file:
		std::string container_filename;
		size_t container_filesize = ~0ull;
		size_t container_fileoffset = 0;
		uint64_t timestamp = 0;

		// Streaming parameters:
		StreamingTexture streaming_texture;
		std::atomic<uint32_t> streaming_resolution{ 0 };
		uint32_t streaming_unload_delay = 0;
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
	size_t Resource::GetScriptHash() const
	{
		if (internal_state == nullptr)
			return 0;
		const ResourceInternal* resourceinternal = (ResourceInternal*)internal_state.get();
		return resourceinternal->script_hash;
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
		resourceinternal->timestamp = 0;
	}

	void Resource::StreamingRequestResolution(uint32_t resolution)
	{
		if (internal_state == nullptr)
		{
			internal_state = std::make_shared<ResourceInternal>();
		}
		ResourceInternal* resourceinternal = (ResourceInternal*)internal_state.get();
		resourceinternal->streaming_resolution.fetch_or(resolution);
	}

	namespace resourcemanager
	{
		static std::mutex locker;
		static std::unordered_map<std::string, std::weak_ptr<ResourceInternal>> resources;
		static Mode mode = Mode::NO_EMBEDDING;

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

		bool LoadResourceDirectly(
			const std::string& name,
			Flags flags,
			const uint8_t* filedata,
			size_t filesize,
			ResourceInternal* resource
		)
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
					return false;
				}
			}

			bool success = false;

			switch (type)
			{
			case DataType::IMAGE:
			{
				GraphicsDevice* device = wi::graphics::GetDevice();
				if (!ext.compare("KTX2"))
				{
					flags &= ~Flags::STREAMING; // disable streaming
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
					flags &= ~Flags::STREAMING; // disable streaming
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
					dds::Header header = dds::read_header(filedata, filesize);
					if (header.is_valid())
					{
						TextureDesc desc;
						desc.array_size = 1;
						desc.bind_flags = BindFlag::SHADER_RESOURCE;
						desc.width = header.width();
						desc.height = header.height();
						desc.depth = header.depth();
						desc.mip_levels = header.mip_levels();
						desc.array_size = header.array_size();
						desc.format = Format::R8G8B8A8_UNORM;
						desc.layout = ResourceState::SHADER_RESOURCE;
						desc.misc_flags = ResourceMiscFlag::TYPED_FORMAT_CASTING;

						if (header.is_cubemap())
						{
							desc.misc_flags |= ResourceMiscFlag::TEXTURECUBE;
						}
						if (desc.mip_levels == 1 || desc.depth > 1 || desc.array_size > 1)
						{
							// don't allow streaming for single mip, array and 3D textures
							flags &= ~Flags::STREAMING;
						}

						auto ddsFormat = header.format();

						switch (ddsFormat)
						{
						case dds::DXGI_FORMAT_R32G32B32A32_FLOAT: desc.format = Format::R32G32B32A32_FLOAT; break;
						case dds::DXGI_FORMAT_R32G32B32A32_UINT: desc.format = Format::R32G32B32A32_UINT; break;
						case dds::DXGI_FORMAT_R32G32B32A32_SINT: desc.format = Format::R32G32B32A32_SINT; break;
						case dds::DXGI_FORMAT_R32G32B32_FLOAT: desc.format = Format::R32G32B32_FLOAT; break;
						case dds::DXGI_FORMAT_R32G32B32_UINT: desc.format = Format::R32G32B32_UINT; break;
						case dds::DXGI_FORMAT_R32G32B32_SINT: desc.format = Format::R32G32B32_SINT; break;
						case dds::DXGI_FORMAT_R16G16B16A16_FLOAT: desc.format = Format::R16G16B16A16_FLOAT; break;
						case dds::DXGI_FORMAT_R16G16B16A16_UNORM: desc.format = Format::R16G16B16A16_UNORM; break;
						case dds::DXGI_FORMAT_R16G16B16A16_UINT: desc.format = Format::R16G16B16A16_UINT; break;
						case dds::DXGI_FORMAT_R16G16B16A16_SNORM: desc.format = Format::R16G16B16A16_SNORM; break;
						case dds::DXGI_FORMAT_R16G16B16A16_SINT: desc.format = Format::R16G16B16A16_SINT; break;
						case dds::DXGI_FORMAT_R32G32_FLOAT: desc.format = Format::R32G32_FLOAT; break;
						case dds::DXGI_FORMAT_R32G32_UINT: desc.format = Format::R32G32_UINT; break;
						case dds::DXGI_FORMAT_R32G32_SINT: desc.format = Format::R32G32_SINT; break;
						case dds::DXGI_FORMAT_R10G10B10A2_UNORM: desc.format = Format::R10G10B10A2_UNORM; break;
						case dds::DXGI_FORMAT_R10G10B10A2_UINT: desc.format = Format::R10G10B10A2_UINT; break;
						case dds::DXGI_FORMAT_R11G11B10_FLOAT: desc.format = Format::R11G11B10_FLOAT; break;
						case dds::DXGI_FORMAT_R9G9B9E5_SHAREDEXP: desc.format = Format::R9G9B9E5_SHAREDEXP; break;
						case dds::DXGI_FORMAT_B8G8R8X8_UNORM: desc.format = Format::B8G8R8A8_UNORM; break;
						case dds::DXGI_FORMAT_B8G8R8A8_UNORM: desc.format = Format::B8G8R8A8_UNORM; break;
						case dds::DXGI_FORMAT_B8G8R8A8_UNORM_SRGB: desc.format = Format::B8G8R8A8_UNORM_SRGB; break;
						case dds::DXGI_FORMAT_R8G8B8A8_UNORM: desc.format = Format::R8G8B8A8_UNORM; break;
						case dds::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: desc.format = Format::R8G8B8A8_UNORM_SRGB; break;
						case dds::DXGI_FORMAT_R8G8B8A8_UINT: desc.format = Format::R8G8B8A8_UINT; break;
						case dds::DXGI_FORMAT_R8G8B8A8_SNORM: desc.format = Format::R8G8B8A8_SNORM; break;
						case dds::DXGI_FORMAT_R8G8B8A8_SINT: desc.format = Format::R8G8B8A8_SINT; break;
						case dds::DXGI_FORMAT_R16G16_FLOAT: desc.format = Format::R16G16_FLOAT; break;
						case dds::DXGI_FORMAT_R16G16_UNORM: desc.format = Format::R16G16_UNORM; break;
						case dds::DXGI_FORMAT_R16G16_UINT: desc.format = Format::R16G16_UINT; break;
						case dds::DXGI_FORMAT_R16G16_SNORM: desc.format = Format::R16G16_SNORM; break;
						case dds::DXGI_FORMAT_R16G16_SINT: desc.format = Format::R16G16_SINT; break;
						case dds::DXGI_FORMAT_D32_FLOAT: desc.format = Format::D32_FLOAT; break;
						case dds::DXGI_FORMAT_R32_FLOAT: desc.format = Format::R32_FLOAT; break;
						case dds::DXGI_FORMAT_R32_UINT: desc.format = Format::R32_UINT; break;
						case dds::DXGI_FORMAT_R32_SINT: desc.format = Format::R32_SINT; break;
						case dds::DXGI_FORMAT_R8G8_UNORM: desc.format = Format::R8G8_UNORM; break;
						case dds::DXGI_FORMAT_R8G8_UINT: desc.format = Format::R8G8_UINT; break;
						case dds::DXGI_FORMAT_R8G8_SNORM: desc.format = Format::R8G8_SNORM; break;
						case dds::DXGI_FORMAT_R8G8_SINT: desc.format = Format::R8G8_SINT; break;
						case dds::DXGI_FORMAT_R16_FLOAT: desc.format = Format::R16_FLOAT; break;
						case dds::DXGI_FORMAT_D16_UNORM: desc.format = Format::D16_UNORM; break;
						case dds::DXGI_FORMAT_R16_UNORM: desc.format = Format::R16_UNORM; break;
						case dds::DXGI_FORMAT_R16_UINT: desc.format = Format::R16_UINT; break;
						case dds::DXGI_FORMAT_R16_SNORM: desc.format = Format::R16_SNORM; break;
						case dds::DXGI_FORMAT_R16_SINT: desc.format = Format::R16_SINT; break;
						case dds::DXGI_FORMAT_R8_UNORM: desc.format = Format::R8_UNORM; break;
						case dds::DXGI_FORMAT_R8_UINT: desc.format = Format::R8_UINT; break;
						case dds::DXGI_FORMAT_R8_SNORM: desc.format = Format::R8_SNORM; break;
						case dds::DXGI_FORMAT_R8_SINT: desc.format = Format::R8_SINT; break;
						case dds::DXGI_FORMAT_BC1_UNORM: desc.format = Format::BC1_UNORM; break;
						case dds::DXGI_FORMAT_BC1_UNORM_SRGB: desc.format = Format::BC1_UNORM_SRGB; break;
						case dds::DXGI_FORMAT_BC2_UNORM: desc.format = Format::BC2_UNORM; break;
						case dds::DXGI_FORMAT_BC2_UNORM_SRGB: desc.format = Format::BC2_UNORM_SRGB; break;
						case dds::DXGI_FORMAT_BC3_UNORM: desc.format = Format::BC3_UNORM; break;
						case dds::DXGI_FORMAT_BC3_UNORM_SRGB: desc.format = Format::BC3_UNORM_SRGB; break;
						case dds::DXGI_FORMAT_BC4_UNORM: desc.format = Format::BC4_UNORM; break;
						case dds::DXGI_FORMAT_BC4_SNORM: desc.format = Format::BC4_SNORM; break;
						case dds::DXGI_FORMAT_BC5_UNORM: desc.format = Format::BC5_UNORM; break;
						case dds::DXGI_FORMAT_BC5_SNORM: desc.format = Format::BC5_SNORM; break;
						case dds::DXGI_FORMAT_BC6H_SF16: desc.format = Format::BC6H_SF16; break;
						case dds::DXGI_FORMAT_BC6H_UF16: desc.format = Format::BC6H_UF16; break;
						case dds::DXGI_FORMAT_BC7_UNORM: desc.format = Format::BC7_UNORM; break;
						case dds::DXGI_FORMAT_BC7_UNORM_SRGB: desc.format = Format::BC7_UNORM_SRGB; break;
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

						if (header.is_1d())
						{
							desc.type = TextureDesc::Type::TEXTURE_1D;
						}
						else if (header.is_3d())
						{
							desc.type = TextureDesc::Type::TEXTURE_3D;
						}

						if (IsFormatBlockCompressed(desc.format))
						{
							desc.width = AlignTo(desc.width, GetFormatBlockSize(desc.format));
							desc.height = AlignTo(desc.height, GetFormatBlockSize(desc.format));
						}

						wi::vector<SubresourceData> initdata_heap;
						SubresourceData initdata_stack[16] = {};
						SubresourceData* initdata = nullptr;

						// Determine if we need heap allocation for initdata, or it is small enough for stack:
						if (desc.array_size * desc.mip_levels < arraysize(initdata_stack))
						{
							initdata = initdata_stack;
						}
						else
						{
							initdata_heap.resize(desc.array_size * desc.mip_levels);
							initdata = initdata_heap.data();
						}

						uint32_t subresource_index = 0;
						for (uint32_t slice = 0; slice < desc.array_size; ++slice)
						{
							for (uint32_t mip = 0; mip < desc.mip_levels; ++mip)
							{
								SubresourceData& subresourceData = initdata[subresource_index++];
								subresourceData.data_ptr = filedata + header.mip_offset(mip, slice);
								subresourceData.row_pitch = header.row_pitch(mip);
								subresourceData.slice_pitch = header.slice_pitch(mip);
							}
						}

						int mip_offset = 0;
						if (has_flag(flags, Flags::STREAMING))
						{
							// Remember full mipcount for streaming:
							resource->streaming_texture.mip_count = desc.mip_levels;
							// For streaming, remember relative memory offsets for mip levels:
							for (uint32_t slice = 0; slice < desc.array_size; ++slice)
							{
								for (uint32_t mip = 0; mip < desc.mip_levels; ++mip)
								{
									auto& streaming_data = resource->streaming_texture.streaming_data[mip];
									streaming_data.data_offset = header.mip_offset(mip, slice);
									streaming_data.row_pitch = header.row_pitch(mip);
									streaming_data.slice_pitch = header.slice_pitch(mip);
								}
							}
							// Reduce mip map count that will be uploaded to GPU:
							while (desc.mip_levels > 1 && desc.depth == 1 && desc.array_size == 1 && ComputeTextureMemorySizeInBytes(desc) > streaming_texture_min_size)
							{
								desc.width >>= 1;
								desc.height >>= 1;
								desc.mip_levels -= 1;
								mip_offset++;
							}
							resource->streaming_texture.min_lod_clamp_absolute = (float)mip_offset;
						}

						success = device->CreateTexture(&desc, initdata + mip_offset, &resource->texture);
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
					flags &= ~Flags::STREAMING; // disable streaming
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
					flags &= ~Flags::STREAMING; // disable streaming
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
								swizzle = { ComponentSwizzle::R, ComponentSwizzle::R, ComponentSwizzle::R, ComponentSwizzle::G };
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
								swizzle = { ComponentSwizzle::R, ComponentSwizzle::R, ComponentSwizzle::R, ComponentSwizzle::G };
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
							SubresourceData init_data[16];
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
				resource->script_hash = wi::helper::string_hash(resource->script.c_str());
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

			if (!resource->filedata.empty() && !has_flag(flags, Flags::IMPORT_RETAIN_FILEDATA) && !has_flag(flags, Flags::IMPORT_DELAY))
			{
				// file data can be discarded:
				resource->filedata.clear();
				resource->filedata.shrink_to_fit();
			}

			return success;
		}

		Resource Load(
			const std::string& name,
			Flags flags,
			const uint8_t* filedata,
			size_t filesize,
			const std::string& container_filename,
			size_t container_fileoffset
		)
		{
			locker.lock();
			std::weak_ptr<ResourceInternal>& weak_resource = resources[name];
			std::shared_ptr<ResourceInternal> resource = weak_resource.lock();

			static bool basis_init = false; // within lock!
			if (!basis_init)
			{
				basis_init = true;
				basist::basisu_transcoder_init();
			}

			uint64_t timestamp = 0;
			if(!container_filename.empty())
			{
				timestamp = wi::helper::FileTimestamp(container_filename);
			}
			else
			{
				timestamp = wi::helper::FileTimestamp(name);
			}

			if (resource == nullptr || resource->timestamp < timestamp)
			{
				resource = std::make_shared<ResourceInternal>();
				resources[name] = resource;
				resource->filename = name;

				// Rememeber the streaming file parameters, which is either the resource filename,
				//	or it can be a specific filename and offset in the case when the file contained multiple resources
				if (container_filename.empty())
				{
					resource->container_filename = name;
				}
				else
				{
					resource->container_filename = container_filename;
				}
				resource->container_filesize = filesize;
				resource->container_fileoffset = container_fileoffset;

				if (filedata != nullptr && resource->filedata.empty() && (has_flag(flags, Flags::IMPORT_RETAIN_FILEDATA) || has_flag(flags, Flags::IMPORT_DELAY)))
				{
					// resource was loaded with external filedata, and we want to retain filedata
					//	this must also happen when using IMPORT_DELAY!
					resource->filedata.resize(filesize);
					std::memcpy(resource->filedata.data(), filedata, filesize);
				}
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
				if (resource->filedata.empty())
				{
					if (!wi::helper::FileRead(resource->container_filename, resource->filedata, resource->container_filesize, resource->container_fileoffset))
					{
						resource.reset();
						return Resource();
					}
				}
				filedata = resource->filedata.data();
				filesize = resource->filedata.size();
			}

			flags |= resource->flags;

			bool success = false;

			if (has_flag(flags, Flags::IMPORT_DELAY))
			{
				success = true;
			}
			else
			{
				success = LoadResourceDirectly(name, flags, filedata, filesize, resource.get());
			}

			if (success)
			{
				resource->flags = flags;
				resource->timestamp = timestamp;

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

		wi::jobsystem::context streaming_ctx;
		wi::vector<std::shared_ptr<ResourceInternal>> streaming_texture_jobs;
		struct StreamingTextureReplace
		{
			std::shared_ptr<ResourceInternal> resource;
			Texture texture;
			int srgb_subresource = -1;
		};
		std::mutex streaming_replacement_mutex;
		wi::vector<StreamingTextureReplace> streaming_texture_replacements;
		float streaming_threshold = 0.8f;
		float streaming_fade_speed = 4;

		void SetStreamingMemoryThreshold(float value)
		{
			streaming_threshold = value;
		}

		float GetStreamingMemoryThreshold()
		{
			return streaming_threshold;
		}

		void UpdateStreamingResources(float dt)
		{
			// If any streaming replacement requests arrived, replace the resources here (main thread):
			streaming_replacement_mutex.lock(); // streaming_replacement_mutex is not a long lock, it can only be held by the single streaming thread, so we don't need to try_lock
			for (auto& replace : streaming_texture_replacements)
			{
				replace.resource->texture = replace.texture;
				replace.resource->srgb_subresource = replace.srgb_subresource;
			}
			streaming_texture_replacements.clear();
			streaming_replacement_mutex.unlock();

			// Update resource min lod clamps smoothly:
			GraphicsDevice* device = GetDevice();
			if (!locker.try_lock()) // Use try lock as this is on the main thread which shouldn't hitch on long locking!
				return; // Streaming is not that important, we can abandon it if some resource loading is holding the lock
			for (auto& x : resources)
			{
				std::weak_ptr<ResourceInternal>& weak_resource = x.second;
				std::shared_ptr<ResourceInternal> resource = weak_resource.lock();
				if (resource != nullptr && resource->texture.IsValid() && has_flag(resource->flags, Flags::STREAMING))
				{
					const TextureDesc& desc = resource->texture.desc;
					const float mip_offset = float(resource->streaming_texture.mip_count - desc.mip_levels);
					float min_lod_clamp_absolute_next = resource->streaming_texture.min_lod_clamp_absolute - dt * streaming_fade_speed;
					min_lod_clamp_absolute_next = std::max(mip_offset, min_lod_clamp_absolute_next);
					if (wi::math::float_equal(min_lod_clamp_absolute_next, resource->streaming_texture.min_lod_clamp_absolute))
						continue;
					resource->streaming_texture.min_lod_clamp_absolute = min_lod_clamp_absolute_next;

					const float min_lod_clamp_relative = min_lod_clamp_absolute_next - mip_offset;

					device->DeleteSubresources(&resource->texture);

					device->CreateSubresource(
						&resource->texture,
						SubresourceType::SRV,
						0, -1,
						0, -1,
						nullptr,
						nullptr,
						nullptr,
						min_lod_clamp_relative
					);
					resource->srgb_subresource = -1;

					Format srgb_format = GetFormatSRGB(desc.format);
					if (srgb_format != Format::UNKNOWN && srgb_format != desc.format)
					{
						resource->srgb_subresource = device->CreateSubresource(
							&resource->texture,
							SubresourceType::SRV,
							0, -1,
							0, -1,
							&srgb_format,
							nullptr,
							nullptr,
							min_lod_clamp_relative
						);
					}
				}
			}

			// If previous streaming jobs were not finished, we cancel this until next frame:
			if (wi::jobsystem::IsBusy(streaming_ctx))
			{
				locker.unlock();
				return;
			}

			streaming_texture_jobs.clear();

			// Gather the streaming jobs:
			for (auto& x : resources)
			{
				std::weak_ptr<ResourceInternal>& weak_resource = x.second;
				std::shared_ptr<ResourceInternal> resource = weak_resource.lock();
				if (resource != nullptr && resource->texture.IsValid() && resource->streaming_texture.mip_count > 1)
				{
					streaming_texture_jobs.push_back(resource);
				}
			}
			locker.unlock();

			if (streaming_texture_jobs.empty())
				return;

			// One low priority thread will be responsible for streaming, to not cause any hitching while rendering:
			streaming_ctx.priority = wi::jobsystem::Priority::Streaming;
			wi::jobsystem::Execute(streaming_ctx, [](wi::jobsystem::JobArgs args) {
				for(auto& resource : streaming_texture_jobs)
				{
					TextureDesc desc = resource->texture.desc;
					uint32_t requested_resolution = resource->streaming_resolution.fetch_and(0); // set to zero while returning prev value
					if (requested_resolution > 0)
					{
						requested_resolution = 1ul << (31ul - firstbithigh((unsigned long)requested_resolution)); // largest power of two
					}
					GraphicsDevice* device = GetDevice();
					const GraphicsDevice::MemoryUsage memory_usage = device->GetMemoryUsage();
					const float memory_percent = float(double(memory_usage.usage) / double(memory_usage.budget));
					const bool memory_shortage = memory_percent > streaming_threshold;
					const bool stream_in = memory_shortage ? false : (requested_resolution >= std::min(desc.width, desc.height));

					int mip_offset = int(resource->streaming_texture.mip_count - desc.mip_levels);
					if (stream_in)
					{
						resource->streaming_unload_delay = 0; // unloading will be immediately halted
						if (mip_offset == 0)
							continue; // There aren't any more mip levels, cancel
						// Mip level streaming IN:
						desc.width <<= 1;
						desc.height <<= 1;
						if (requested_resolution < std::min(desc.width, desc.height))
							continue; // Increased resolution would be too much, cancel
						desc.mip_levels++;
						mip_offset--;
					}
					else
					{
						resource->streaming_unload_delay++; // one more frame that this wants to unload
						if (!memory_shortage && resource->streaming_unload_delay < 255)
							continue; // only unload mips if it's been wanting to unload for a couple frames, or there is memory shortage
						if (ComputeTextureMemorySizeInBytes(desc) <= streaming_texture_min_size)
							continue; // Don't reduce the texture below, because of 4KB alignment, this would not reduce memory usage further
						// Mip level streaming OUT:
						desc.width >>= 1;
						desc.height >>= 1;
						desc.mip_levels--;
						mip_offset++;
					}
					if (desc.mip_levels <= resource->streaming_texture.mip_count)
					{
						// memory offset of the first mip level in current streaming range:
						const size_t mip_data_offset = resource->streaming_texture.streaming_data[mip_offset].data_offset;
						const uint8_t* firstmipdata = resource->filedata.data();

						static wi::vector<uint8_t> streaming_file; // make this static to not reallocate for each file loading
						if (firstmipdata == nullptr)
						{
							// If file data is not available, then open the file partially with the streaming file parameters:
							size_t filesize = resource->container_filesize - mip_data_offset;
							size_t fileoffset = resource->container_fileoffset + mip_data_offset;
							if (!wi::helper::FileRead(
								resource->container_filename,
								streaming_file,
								filesize,
								fileoffset
							))
							{
								continue;
							}
							firstmipdata = streaming_file.data();
						}
						else
						{
							// If file data is available, we can use that for streaming:
							firstmipdata += mip_data_offset;
						}

						// Convert relative to absolute GPU initialization data
						SubresourceData initdata[16] = {};
						for (uint32_t mip = 0; mip < desc.mip_levels; ++mip)
						{
							auto& streaming_data = resource->streaming_texture.streaming_data[mip_offset + mip];
							initdata[mip].data_ptr = firstmipdata + streaming_data.data_offset - mip_data_offset;
							initdata[mip].row_pitch = streaming_data.row_pitch;
							initdata[mip].slice_pitch = streaming_data.slice_pitch;
						}

						// The replacement struct will store the newly created texture until replacement can be made later:
						StreamingTextureReplace replace;
						replace.resource = resource;
						replace.srgb_subresource = -1;
						bool success = device->CreateTexture(&desc, initdata, &replace.texture);
						assert(success);
						device->SetName(&replace.texture, resource->filename.c_str());

						Format srgb_format = GetFormatSRGB(desc.format);
						if (srgb_format != Format::UNKNOWN && srgb_format != desc.format)
						{
							replace.srgb_subresource = device->CreateSubresource(
								&replace.texture,
								SubresourceType::SRV,
								0, -1,
								0, -1,
								&srgb_format
							);
						}

						streaming_replacement_mutex.lock();
						streaming_texture_replacements.push_back(replace);
						streaming_replacement_mutex.unlock();
					}
				}
			});
		}

		bool CheckResourcesOutdated()
		{
			std::scoped_lock lck(locker);

			for (auto& x : resources)
			{
				const std::string& name = x.first;
				auto resourceinternal = x.second.lock();
				if (resourceinternal == nullptr)
					continue;

				uint64_t timestamp = wi::helper::FileTimestamp(resourceinternal->filename);
				if (resourceinternal->timestamp < timestamp)
					return true;
			}
			return false;
		}

		void ReloadOutdatedResources()
		{
			std::scoped_lock lck(locker);

			for (auto& x : resources)
			{
				auto resourceinternal = x.second.lock();
				if (resourceinternal == nullptr)
					continue;

				uint64_t timestamp = wi::helper::FileTimestamp(resourceinternal->filename);
				if (resourceinternal->timestamp < timestamp)
				{
					wi::vector<uint8_t> filedata;
					if (wi::helper::FileRead(resourceinternal->filename, filedata))
					{
						if (resourceinternal->streaming_texture.mip_count > 1)
							wi::jobsystem::Wait(streaming_ctx); // reloading a resource that is potentially streaming needs to wait for current streaming job to end
						if (LoadResourceDirectly(resourceinternal->filename, resourceinternal->flags, filedata.data(), filedata.size(), resourceinternal.get()))
						{
							resourceinternal->timestamp = timestamp;
							resourceinternal->container_filename = resourceinternal->filename;
							resourceinternal->container_fileoffset = 0;
							resourceinternal->container_filesize = ~0ull;
							wi::backlog::post("[resourcemanager] reload success: " + resourceinternal->filename);
						}
						else
						{
							wi::backlog::post("[resourcemanager] reload failure - LoadResourceDirectly returned false: " + resourceinternal->filename, wi::backlog::LogLevel::Error);
						}
					}
					else
					{
						wi::backlog::post("[resourcemanager] reload failure - file data could not be read: " + resourceinternal->filename, wi::backlog::LogLevel::Error);
					}
				}
			}
		}

		void Serialize_READ(wi::Archive& archive, ResourceSerializer& seri)
		{
			assert(archive.IsReadMode());
			size_t serializable_count = 0;
			archive >> serializable_count;

			struct TempResource
			{
				std::string name;
				const uint8_t* filedata = nullptr;
				size_t filesize = 0;
			};
			wi::vector<TempResource> temp_resources;
			temp_resources.resize(serializable_count);

			wi::jobsystem::context ctx;
			ctx.priority = wi::jobsystem::Priority::Low;

			for (size_t i = 0; i < serializable_count; ++i)
			{
				auto& resource = temp_resources[i];

				archive >> resource.name;
				uint32_t flags_temp;
				archive >> flags_temp;
				// Note: flags not applied here, but they must be read
				//	We don't apply the flags, because they will be requested later by for example materials
				//	If we would apply flags here, then flags from previous session would be applied, that maybe we no longer want (for example RETAIN_FILEDATA)

				// We don't read the file data from archive into a vector like usual, instead map the vector,
				//  this is much faster and we don't need to retain this data after archive lifetime
				archive.MapVector(resource.filedata, resource.filesize);

				size_t file_offset = archive.GetPos() - resource.filesize;

				resource.name = archive.GetSourceDirectory() + resource.name;

				if (Contains(resource.name))
					continue;

				// "Loading" the resource can happen asynchronously to serialization of file data, to improve performance
				wi::jobsystem::Execute(ctx, [i, &temp_resources, &seri, &archive, file_offset](wi::jobsystem::JobArgs args) {
					auto& tmp_resource = temp_resources[i];
					auto res = Load(
						tmp_resource.name,
						Flags::IMPORT_DELAY,
						tmp_resource.filedata,
						tmp_resource.filesize,
						archive.GetSourceFileName(),
						file_offset
					);
					static std::mutex seri_locker;
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

			if (mode == Mode::NO_EMBEDDING)
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
					if (resource != nullptr)
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

					if (resource != nullptr)
					{
						std::string name = it->first;
						wi::helper::MakePathRelative(archive.GetSourceDirectory(), name);

						if (resource->filedata.empty())
						{
							wi::helper::FileRead(
								resource->container_filename,
								resource->filedata,
								resource->container_filesize,
								resource->container_fileoffset
							);
						}

						archive << name;
						archive << (uint32_t)resource->flags;
						archive << resource->filedata;

						if (!archive.GetSourceFileName().empty())
						{
							// Refresh the container file properties to the current file:
							//	The old file offsets could get stale otherwise if it's overwritten
							resource->container_filename = archive.GetSourceFileName();
							resource->container_fileoffset = archive.GetPos() - resource->filedata.size();
							resource->container_filesize = resource->filedata.size();
							if (!has_flag(resource->flags, Flags::IMPORT_RETAIN_FILEDATA))
							{
								resource->filedata.clear();
								resource->filedata.shrink_to_fit();
							}
						}
					}
				}
			}
			locker.unlock();
		}

	}

}
