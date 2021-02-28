#include "wiResourceManager.h"
#include "wiRenderer.h"
#include "wiHelper.h"
#include "wiTextureHelper.h"

#include "Utility/stb_image.h"
#include "Utility/tinyddsloader.h"

#include <algorithm>

using namespace wiGraphics;

namespace wiResourceManager
{
	std::mutex locker;
	std::unordered_map<std::string, std::weak_ptr<wiResource>> resources;
	MODE mode = MODE_DISCARD_FILEDATA_AFTER_LOAD;

	void SetMode(MODE param)
	{
		mode = param;
	}
	MODE GetMode()
	{
		return mode;
	}

	static const std::unordered_map<std::string, wiResource::DATA_TYPE> types = {
		std::make_pair("JPG", wiResource::IMAGE),
		std::make_pair("JPEG", wiResource::IMAGE),
		std::make_pair("PNG", wiResource::IMAGE),
		std::make_pair("BMP", wiResource::IMAGE),
		std::make_pair("DDS", wiResource::IMAGE),
		std::make_pair("TGA", wiResource::IMAGE),
		std::make_pair("WAV", wiResource::SOUND),
		std::make_pair("OGG", wiResource::SOUND),
	};

	std::shared_ptr<wiResource> Load(const std::string& name, uint32_t flags, const uint8_t* filedata, size_t filesize)
	{
		if (mode == MODE_DISCARD_FILEDATA_AFTER_LOAD)
		{
			flags &= ~IMPORT_RETAIN_FILEDATA;
		}

		locker.lock();
		std::weak_ptr<wiResource>& weak_resource = resources[name];
		std::shared_ptr<wiResource> resource = weak_resource.lock();

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
			GraphicsDevice* device = wiRenderer::GetDevice();
			if (!ext.compare(std::string("DDS")))
			{
				// Load dds

				tinyddsloader::DDSFile dds;
				auto result = dds.Load(filedata, filesize);

				if (result == tinyddsloader::Result::Success)
				{
					TextureDesc desc;
					desc.ArraySize = 1;
					desc.BindFlags = BIND_SHADER_RESOURCE;
					desc.CPUAccessFlags = 0;
					desc.Height = dds.GetWidth();
					desc.Width = dds.GetHeight();
					desc.Depth = dds.GetDepth();
					desc.MipLevels = dds.GetMipCount();
					desc.ArraySize = dds.GetArraySize();
					desc.MiscFlags = 0;
					desc.Usage = USAGE_IMMUTABLE;
					desc.Format = FORMAT_R8G8B8A8_UNORM;
					desc.layout = IMAGE_LAYOUT_SHADER_RESOURCE;

					if (dds.IsCubemap())
					{
						desc.MiscFlags |= RESOURCE_MISC_TEXTURECUBE;
					}

					auto ddsFormat = dds.GetFormat();

					switch (ddsFormat)
					{
					case tinyddsloader::DDSFile::DXGIFormat::R32G32B32A32_Float: desc.Format = FORMAT_R32G32B32A32_FLOAT; break;
					case tinyddsloader::DDSFile::DXGIFormat::R32G32B32A32_UInt: desc.Format = FORMAT_R32G32B32A32_UINT; break;
					case tinyddsloader::DDSFile::DXGIFormat::R32G32B32A32_SInt: desc.Format = FORMAT_R32G32B32A32_SINT; break;
					case tinyddsloader::DDSFile::DXGIFormat::R32G32B32_Float: desc.Format = FORMAT_R32G32B32_FLOAT; break;
					case tinyddsloader::DDSFile::DXGIFormat::R32G32B32_UInt: desc.Format = FORMAT_R32G32B32_UINT; break;
					case tinyddsloader::DDSFile::DXGIFormat::R32G32B32_SInt: desc.Format = FORMAT_R32G32B32_SINT; break;
					case tinyddsloader::DDSFile::DXGIFormat::R16G16B16A16_Float: desc.Format = FORMAT_R16G16B16A16_FLOAT; break;
					case tinyddsloader::DDSFile::DXGIFormat::R16G16B16A16_UNorm: desc.Format = FORMAT_R16G16B16A16_UNORM; break;
					case tinyddsloader::DDSFile::DXGIFormat::R16G16B16A16_UInt: desc.Format = FORMAT_R16G16B16A16_UINT; break;
					case tinyddsloader::DDSFile::DXGIFormat::R16G16B16A16_SNorm: desc.Format = FORMAT_R16G16B16A16_SNORM; break;
					case tinyddsloader::DDSFile::DXGIFormat::R16G16B16A16_SInt: desc.Format = FORMAT_R16G16B16A16_SINT; break;
					case tinyddsloader::DDSFile::DXGIFormat::R32G32_Float: desc.Format = FORMAT_R32G32_FLOAT; break;
					case tinyddsloader::DDSFile::DXGIFormat::R32G32_UInt: desc.Format = FORMAT_R32G32_UINT; break;
					case tinyddsloader::DDSFile::DXGIFormat::R32G32_SInt: desc.Format = FORMAT_R32G32_SINT; break;
					case tinyddsloader::DDSFile::DXGIFormat::R10G10B10A2_UNorm: desc.Format = FORMAT_R10G10B10A2_UNORM; break;
					case tinyddsloader::DDSFile::DXGIFormat::R10G10B10A2_UInt: desc.Format = FORMAT_R10G10B10A2_UINT; break;
					case tinyddsloader::DDSFile::DXGIFormat::R11G11B10_Float: desc.Format = FORMAT_R11G11B10_FLOAT; break;
					case tinyddsloader::DDSFile::DXGIFormat::B8G8R8A8_UNorm: desc.Format = FORMAT_B8G8R8A8_UNORM; break;
					case tinyddsloader::DDSFile::DXGIFormat::B8G8R8A8_UNorm_SRGB: desc.Format = FORMAT_B8G8R8A8_UNORM_SRGB; break;
					case tinyddsloader::DDSFile::DXGIFormat::R8G8B8A8_UNorm: desc.Format = FORMAT_R8G8B8A8_UNORM; break;
					case tinyddsloader::DDSFile::DXGIFormat::R8G8B8A8_UNorm_SRGB: desc.Format = FORMAT_R8G8B8A8_UNORM_SRGB; break;
					case tinyddsloader::DDSFile::DXGIFormat::R8G8B8A8_UInt: desc.Format = FORMAT_R8G8B8A8_UINT; break;
					case tinyddsloader::DDSFile::DXGIFormat::R8G8B8A8_SNorm: desc.Format = FORMAT_R8G8B8A8_SNORM; break;
					case tinyddsloader::DDSFile::DXGIFormat::R8G8B8A8_SInt: desc.Format = FORMAT_R8G8B8A8_SINT; break;
					case tinyddsloader::DDSFile::DXGIFormat::R16G16_Float: desc.Format = FORMAT_R16G16_FLOAT; break;
					case tinyddsloader::DDSFile::DXGIFormat::R16G16_UNorm: desc.Format = FORMAT_R16G16_UNORM; break;
					case tinyddsloader::DDSFile::DXGIFormat::R16G16_UInt: desc.Format = FORMAT_R16G16_UINT; break;
					case tinyddsloader::DDSFile::DXGIFormat::R16G16_SNorm: desc.Format = FORMAT_R16G16_SNORM; break;
					case tinyddsloader::DDSFile::DXGIFormat::R16G16_SInt: desc.Format = FORMAT_R16G16_SINT; break;
					case tinyddsloader::DDSFile::DXGIFormat::D32_Float: desc.Format = FORMAT_D32_FLOAT; break;
					case tinyddsloader::DDSFile::DXGIFormat::R32_Float: desc.Format = FORMAT_R32_FLOAT; break;
					case tinyddsloader::DDSFile::DXGIFormat::R32_UInt: desc.Format = FORMAT_R32_UINT; break;
					case tinyddsloader::DDSFile::DXGIFormat::R32_SInt: desc.Format = FORMAT_R32_SINT; break;
					case tinyddsloader::DDSFile::DXGIFormat::R8G8_UNorm: desc.Format = FORMAT_R8G8_UNORM; break;
					case tinyddsloader::DDSFile::DXGIFormat::R8G8_UInt: desc.Format = FORMAT_R8G8_UINT; break;
					case tinyddsloader::DDSFile::DXGIFormat::R8G8_SNorm: desc.Format = FORMAT_R8G8_SNORM; break;
					case tinyddsloader::DDSFile::DXGIFormat::R8G8_SInt: desc.Format = FORMAT_R8G8_SINT; break;
					case tinyddsloader::DDSFile::DXGIFormat::R16_Float: desc.Format = FORMAT_R16_FLOAT; break;
					case tinyddsloader::DDSFile::DXGIFormat::D16_UNorm: desc.Format = FORMAT_D16_UNORM; break;
					case tinyddsloader::DDSFile::DXGIFormat::R16_UNorm: desc.Format = FORMAT_R16_UNORM; break;
					case tinyddsloader::DDSFile::DXGIFormat::R16_UInt: desc.Format = FORMAT_R16_UINT; break;
					case tinyddsloader::DDSFile::DXGIFormat::R16_SNorm: desc.Format = FORMAT_R16_SNORM; break;
					case tinyddsloader::DDSFile::DXGIFormat::R16_SInt: desc.Format = FORMAT_R16_SINT; break;
					case tinyddsloader::DDSFile::DXGIFormat::R8_UNorm: desc.Format = FORMAT_R8_UNORM; break;
					case tinyddsloader::DDSFile::DXGIFormat::R8_UInt: desc.Format = FORMAT_R8_UINT; break;
					case tinyddsloader::DDSFile::DXGIFormat::R8_SNorm: desc.Format = FORMAT_R8_SNORM; break;
					case tinyddsloader::DDSFile::DXGIFormat::R8_SInt: desc.Format = FORMAT_R8_SINT; break;
					case tinyddsloader::DDSFile::DXGIFormat::BC1_UNorm: desc.Format = FORMAT_BC1_UNORM; break;
					case tinyddsloader::DDSFile::DXGIFormat::BC1_UNorm_SRGB: desc.Format = FORMAT_BC1_UNORM_SRGB; break;
					case tinyddsloader::DDSFile::DXGIFormat::BC2_UNorm: desc.Format = FORMAT_BC2_UNORM; break;
					case tinyddsloader::DDSFile::DXGIFormat::BC2_UNorm_SRGB: desc.Format = FORMAT_BC2_UNORM_SRGB; break;
					case tinyddsloader::DDSFile::DXGIFormat::BC3_UNorm: desc.Format = FORMAT_BC3_UNORM; break;
					case tinyddsloader::DDSFile::DXGIFormat::BC3_UNorm_SRGB: desc.Format = FORMAT_BC3_UNORM_SRGB; break;
					case tinyddsloader::DDSFile::DXGIFormat::BC4_UNorm: desc.Format = FORMAT_BC4_UNORM; break;
					case tinyddsloader::DDSFile::DXGIFormat::BC4_SNorm: desc.Format = FORMAT_BC4_SNORM; break;
					case tinyddsloader::DDSFile::DXGIFormat::BC5_UNorm: desc.Format = FORMAT_BC5_UNORM; break;
					case tinyddsloader::DDSFile::DXGIFormat::BC5_SNorm: desc.Format = FORMAT_BC5_SNORM; break;
					case tinyddsloader::DDSFile::DXGIFormat::BC7_UNorm: desc.Format = FORMAT_BC7_UNORM; break;
					case tinyddsloader::DDSFile::DXGIFormat::BC7_UNorm_SRGB: desc.Format = FORMAT_BC7_UNORM_SRGB; break;
					default:
						assert(0); // incoming format is not supported 
						break;
					}

					std::vector<SubresourceData> InitData;
					for (uint32_t arrayIndex = 0; arrayIndex < desc.ArraySize; ++arrayIndex)
					{
						for (uint32_t mip = 0; mip < desc.MipLevels; ++mip)
						{
							auto imageData = dds.GetImageData(mip, arrayIndex);
							SubresourceData subresourceData;
							subresourceData.pSysMem = imageData->m_mem;
							subresourceData.SysMemPitch = imageData->m_memPitch;
							subresourceData.SysMemSlicePitch = imageData->m_memSlicePitch;
							InitData.push_back(subresourceData);
						}
					}

					auto dim = dds.GetTextureDimension();
					switch (dim)
					{
					case tinyddsloader::DDSFile::TextureDimension::Texture1D:
					{
						desc.type = TextureDesc::TEXTURE_1D;
					}
					break;
					case tinyddsloader::DDSFile::TextureDimension::Texture2D:
					{
						desc.type = TextureDesc::TEXTURE_2D;
					}
					break;
					case tinyddsloader::DDSFile::TextureDimension::Texture3D:
					{
						desc.type = TextureDesc::TEXTURE_3D;
					}
					break;
					default:
						assert(0);
						break;
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
					desc.Height = uint32_t(height);
					desc.Width = uint32_t(width);
					desc.layout = IMAGE_LAYOUT_SHADER_RESOURCE;

					if (flags & IMPORT_COLORGRADINGLUT)
					{
						if (desc.type != TextureDesc::TEXTURE_2D ||
							desc.Width != 256 ||
							desc.Height != 16)
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

							desc.type = TextureDesc::TEXTURE_3D;
							desc.Width = 16;
							desc.Height = 16;
							desc.Depth = 16;
							desc.Format = FORMAT_R8G8B8A8_UNORM;
							desc.BindFlags = BIND_SHADER_RESOURCE;
							SubresourceData InitData;
							InitData.pSysMem = data;
							InitData.SysMemPitch = 16 * sizeof(uint32_t);
							InitData.SysMemSlicePitch = 16 * InitData.SysMemPitch;
							success = device->CreateTexture(&desc, &InitData, &resource->texture);
							device->SetName(&resource->texture, name.c_str());
						}
					}
					else
					{
						desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
						desc.CPUAccessFlags = 0;
						desc.Format = FORMAT_R8G8B8A8_UNORM;
						desc.MipLevels = (uint32_t)log2(std::max(width, height)) + 1;
						desc.MiscFlags = 0;
						desc.Usage = USAGE_DEFAULT;
						desc.layout = IMAGE_LAYOUT_SHADER_RESOURCE;

						uint32_t mipwidth = width;
						std::vector<SubresourceData> InitData(desc.MipLevels);
						for (uint32_t mip = 0; mip < desc.MipLevels; ++mip)
						{
							InitData[mip].pSysMem = rgb; // attention! we don't fill the mips here correctly, just always point to the mip0 data by default. Mip levels will be created using compute shader when needed!
							InitData[mip].SysMemPitch = static_cast<uint32_t>(mipwidth * channelCount);
							mipwidth = std::max(1u, mipwidth / 2);
						}

						success = device->CreateTexture(&desc, InitData.data(), &resource->texture);
						device->SetName(&resource->texture, name.c_str());

						for (uint32_t i = 0; i < resource->texture.desc.MipLevels; ++i)
						{
							int subresource_index;
							subresource_index = device->CreateSubresource(&resource->texture, SRV, 0, 1, i, 1);
							assert(subresource_index == i);
							subresource_index = device->CreateSubresource(&resource->texture, UAV, 0, 1, i, 1);
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

			if (type == wiResource::IMAGE && resource->texture.desc.MipLevels > 1 && resource->texture.desc.BindFlags & BIND_UNORDERED_ACCESS)
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
