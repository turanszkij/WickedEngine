#include "wiResourceManager.h"
#include "wiRenderer.h"
#include "wiHelper.h"
#include "wiTextureHelper.h"

#include "Utility/stb_image.h"
#include "Utility/tinyddsloader.h"

#include <algorithm>

using namespace wiGraphics;

wiResource::~wiResource()
{
	if (data != nullptr)
	{
		switch (type)
		{
		case wiResource::IMAGE:
			delete texture;
			break;
		case wiResource::SOUND:
			delete sound;
			break;
		};
	}
}

namespace wiResourceManager
{
	std::mutex locker;
	std::unordered_map<std::string, std::weak_ptr<wiResource>> resources;

	static const std::unordered_map<std::string, wiResource::DATA_TYPE> types = {
		std::make_pair("JPG", wiResource::IMAGE),
		std::make_pair("PNG", wiResource::IMAGE),
		std::make_pair("DDS", wiResource::IMAGE),
		std::make_pair("TGA", wiResource::IMAGE),
		std::make_pair("WAV", wiResource::SOUND),
		std::make_pair("OGG", wiResource::SOUND),
	};

	std::shared_ptr<wiResource> Load(const std::string& name)
	{
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

		std::vector<uint8_t> filedata;
		if (!wiHelper::FileRead(name, filedata))
		{
			resource.reset();
			return nullptr;
		}

		std::string ext = wiHelper::toUpper(name.substr(name.length() - 3, name.length()));
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

		void* success = nullptr;

		switch (type)
		{
		case wiResource::IMAGE:
		{
			if (!ext.compare(std::string("DDS")))
			{
				// Load dds

				tinyddsloader::DDSFile dds;
				auto result = dds.Load(std::move(filedata));

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

					Texture* image = new Texture;
					wiRenderer::GetDevice()->CreateTexture(&desc, InitData.data(), image);
					wiRenderer::GetDevice()->SetName(image, name.c_str());
					success = image;
				}
				else assert(0); // failed to load DDS

			}
			else
			{
				// png, tga, jpg, etc. loader:

				const int channelCount = 4;
				int width, height, bpp;
				unsigned char* rgb = stbi_load_from_memory(filedata.data(), (int)filedata.size(), &width, &height, &bpp, channelCount);

				if (rgb != nullptr)
				{
					GraphicsDevice* device = wiRenderer::GetDevice();

					TextureDesc desc;
					desc.ArraySize = 1;
					desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
					desc.CPUAccessFlags = 0;
					desc.Format = FORMAT_R8G8B8A8_UNORM;
					desc.Height = static_cast<uint32_t>(height);
					desc.Width = static_cast<uint32_t>(width);
					desc.MipLevels = (uint32_t)log2(std::max(width, height)) + 1;
					desc.MiscFlags = 0;
					desc.Usage = USAGE_DEFAULT;

					uint32_t mipwidth = width;
					std::vector<SubresourceData> InitData(desc.MipLevels);
					for (uint32_t mip = 0; mip < desc.MipLevels; ++mip)
					{
						InitData[mip].pSysMem = rgb; // attention! we don't fill the mips here correctly, just always point to the mip0 data by default. Mip levels will be created using compute shader when needed!
						InitData[mip].SysMemPitch = static_cast<uint32_t>(mipwidth * channelCount);
						mipwidth = std::max(1u, mipwidth / 2);
					}

					Texture* image = new Texture;
					device->CreateTexture(&desc, InitData.data(), image);
					device->SetName(image, name.c_str());

					for (uint32_t i = 0; i < image->GetDesc().MipLevels; ++i)
					{
						int subresource_index;
						subresource_index = device->CreateSubresource(image, SRV, 0, 1, i, 1);
						assert(subresource_index == i);
						subresource_index = device->CreateSubresource(image, UAV, 0, 1, i, 1);
						assert(subresource_index == i);
					}

					success = image;
				}

				stbi_image_free(rgb);
			}
		}
		break;
		case wiResource::SOUND:
		{
			wiAudio::Sound* sound = new wiAudio::Sound;
			if (wiAudio::CreateSound(filedata, sound))
			{
				success = sound;
			}
		}
		break;
		};

		if (success != nullptr)
		{
			resource->data = success;
			resource->type = type;

			if (type == wiResource::IMAGE && resource->texture->GetDesc().MipLevels > 1 && resource->texture->GetDesc().BindFlags & BIND_UNORDERED_ACCESS)
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
			result = resource != nullptr && resource->data != nullptr;
		}
		locker.unlock();
		return result;
	}

	std::shared_ptr<wiResource> Register(const std::string& name, void* data, wiResource::DATA_TYPE data_type)
	{
		std::shared_ptr<wiResource> resource;

		locker.lock();
		auto it = resources.find(name);
		if (it == resources.end() || it->second.lock() == nullptr)
		{
			resource = std::make_shared<wiResource>();
			resource->data = data;
			resource->type = data_type;
			resources.insert(make_pair(name, resource));
		}
		else
		{
			resource = it->second.lock();
		}
		locker.unlock();

		return resource;
	}

	void Clear()
	{
		locker.lock();
		resources.clear();
		locker.unlock();
	}

}
