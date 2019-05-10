#include "wiResourceManager.h"
#include "wiRenderer.h"
#include "wiSound.h"
#include "wiHelper.h"
#include "wiTextureHelper.h"

#include "Utility/stb_image.h"
#include "Utility/tinyddsloader.h"

#include <algorithm>

using namespace std;
using namespace wiGraphics;

static const std::unordered_map<std::string, wiResourceManager::Data_Type> types = {
	make_pair("JPG", wiResourceManager::IMAGE_2D),
	make_pair("PNG", wiResourceManager::IMAGE_2D),
	make_pair("DDS", wiResourceManager::IMAGE_2D),
	make_pair("TGA", wiResourceManager::IMAGE_2D),
	make_pair("WAV", wiResourceManager::SOUND)
};


wiResourceManager& wiResourceManager::GetGlobal()
{
	static wiResourceManager globalResources;
	return globalResources;
}
wiResourceManager& wiResourceManager::GetShaderManager()
{
	static wiResourceManager shaderManager;
	return shaderManager;
}


wiResourceManager::Resource wiResourceManager::get(const wiHashString& name, bool incRefCount)
{
	lock.lock();
	auto& it = resources.find(name);
	if (it != resources.end())
	{
		if(incRefCount)
			it->second.refCount++;
		lock.unlock();
		return it->second;
	}

	lock.unlock();
	return Resource();
}

const void* wiResourceManager::add(const wiHashString& name, Data_Type newType)
{
	Resource res = get(name,true);
	if(res.type == EMPTY)
	{
		string nameStr = name.GetString();
		string ext = wiHelper::toUpper(nameStr.substr(nameStr.length() - 3, nameStr.length()));
		Data_Type type;

		// dynamic type selection:
		if(newType==Data_Type::DYNAMIC){
			auto& it = types.find(ext);
			if(it!=types.end())
				type = it->second;
			else 
				return nullptr;
		}
		else 
			type = newType;

		void* success = nullptr;

		switch(type)
		{
		case Data_Type::IMAGE_1D:
		case Data_Type::IMAGE_2D:
		case Data_Type::IMAGE_3D:
		{
			if (!ext.compare(std::string("DDS")))
			{
				// Load dds

				tinyddsloader::DDSFile dds;
				auto result = dds.Load(nameStr.c_str());

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
					case tinyddsloader::DDSFile::DXGIFormat::B8G8R8A8_UNorm_SRGB: desc.Format = FORMAT_B8G8R8A8_UNORM; break;
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
					for (UINT arrayIndex = 0; arrayIndex < desc.ArraySize; ++arrayIndex)
					{
						for (UINT mip = 0; mip < desc.MipLevels; ++mip)
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
						Texture1D* image = new Texture1D;
						HRESULT hr = wiRenderer::GetDevice()->CreateTexture1D(&desc, InitData.data(), image);
						assert(SUCCEEDED(hr));
						wiRenderer::GetDevice()->SetName(image, nameStr);
						success = image;
						type = Data_Type::IMAGE_1D;
					}
					break;
					case tinyddsloader::DDSFile::TextureDimension::Texture2D:
					{
						Texture2D* image = new Texture2D;
						HRESULT hr = wiRenderer::GetDevice()->CreateTexture2D(&desc, InitData.data(), image);
						assert(SUCCEEDED(hr));
						wiRenderer::GetDevice()->SetName(image, nameStr);
						success = image;
						type = Data_Type::IMAGE_2D;
					}
					break;
					case tinyddsloader::DDSFile::TextureDimension::Texture3D:
					{
						Texture3D* image = new Texture3D;
						HRESULT hr = wiRenderer::GetDevice()->CreateTexture3D(&desc, InitData.data(), image);
						assert(SUCCEEDED(hr));
						wiRenderer::GetDevice()->SetName(image, nameStr);
						success = image;
						type = Data_Type::IMAGE_3D;
					}
					break;
					default:
						assert(0);
						break;
					}
				}
				else assert(0); // failed to load DDS

			}
			else
			{
				// png, tga, jpg, etc. loader:

				const int channelCount = 4;
				int width, height, bpp;
				unsigned char* rgb = stbi_load(nameStr.c_str(), &width, &height, &bpp, channelCount);

				if (rgb != nullptr)
				{
					TextureDesc desc;
					desc.ArraySize = 1;
					desc.BindFlags = BIND_SHADER_RESOURCE | BIND_UNORDERED_ACCESS;
					desc.CPUAccessFlags = 0;
					desc.Format = FORMAT_R8G8B8A8_UNORM;
					desc.Height = static_cast<uint32_t>(height);
					desc.Width = static_cast<uint32_t>(width);
					desc.MipLevels = (UINT)log2(std::max(width, height));
					desc.MiscFlags = 0;
					desc.Usage = USAGE_DEFAULT;

					UINT mipwidth = width;
					SubresourceData* InitData = new SubresourceData[desc.MipLevels];
					for (UINT mip = 0; mip < desc.MipLevels; ++mip)
					{
						InitData[mip].pSysMem = rgb;
						InitData[mip].SysMemPitch = static_cast<UINT>(mipwidth * channelCount);
						mipwidth = std::max(1u, mipwidth / 2);
					}

					Texture2D* image = new Texture2D;
					image->RequestIndependentShaderResourcesForMIPs(true);
					image->RequestIndependentUnorderedAccessResourcesForMIPs(true);
					HRESULT hr = wiRenderer::GetDevice()->CreateTexture2D(&desc, InitData, image);
					assert(SUCCEEDED(hr));
					wiRenderer::GetDevice()->SetName(image, nameStr);

					if (image != nullptr && image->GetDesc().MipLevels > 1)
					{
						wiRenderer::AddDeferredMIPGen(image);
					}

					success = image;
				}

				stbi_image_free(rgb);
			}
		}
		break;
		case Data_Type::SOUND:
		{
			success = new wiSoundEffect(name.GetString());
		}
		break;
		case Data_Type::MUSIC:
		{
			success = new wiMusic(name.GetString());
		}
		break;
		case Data_Type::VERTEXSHADER:
		{
			vector<uint8_t> buffer;
			if (wiHelper::readByteData(nameStr, buffer)) {
				VertexShader* shader = new VertexShader;
				wiRenderer::GetDevice()->CreateVertexShader(buffer.data(), buffer.size(), shader);
				success = shader;
			}
		}
		break;
		case Data_Type::PIXELSHADER:
		{
			vector<uint8_t> buffer;
			if (wiHelper::readByteData(nameStr, buffer)){
				PixelShader* shader = new PixelShader;
				wiRenderer::GetDevice()->CreatePixelShader(buffer.data(), buffer.size(), shader);
				success = shader;
			}
		}
		break;
		case Data_Type::GEOMETRYSHADER:
		{
			vector<uint8_t> buffer;
			if (wiHelper::readByteData(nameStr, buffer)){
				GeometryShader* shader = new GeometryShader;
				wiRenderer::GetDevice()->CreateGeometryShader(buffer.data(), buffer.size(), shader);
				success = shader;
			}
		}
		break;
		case Data_Type::HULLSHADER:
		{
			vector<uint8_t> buffer;
			if (wiHelper::readByteData(nameStr, buffer)){
				HullShader* shader = new HullShader;
				wiRenderer::GetDevice()->CreateHullShader(buffer.data(), buffer.size(), shader);
				success = shader;
			}
		}
		break;
		case Data_Type::DOMAINSHADER:
		{
			vector<uint8_t> buffer;
			if (wiHelper::readByteData(nameStr, buffer)){
				DomainShader* shader = new DomainShader;
				wiRenderer::GetDevice()->CreateDomainShader(buffer.data(), buffer.size(), shader);
				success = shader;
			}
		}
		break;
		case Data_Type::COMPUTESHADER:
		{
			vector<uint8_t> buffer;
			if (wiHelper::readByteData(nameStr, buffer)) {
				ComputeShader* shader = new ComputeShader;
				wiRenderer::GetDevice()->CreateComputeShader(buffer.data(), buffer.size(), shader);
				success = shader;
			}
		}
		break;
		default:
			break;
		};

		if (success != nullptr)
		{
			lock.lock();
			Resource resource;
			resource.data = success;
			resource.type = type;
			resource.refCount = 1;
			resources.insert(make_pair(name, resource));
			lock.unlock();
		}

		return success;
	}

	return res.data;
}

bool wiResourceManager::del(const wiHashString& name, bool forceDelete)
{
	lock.lock();
	Resource res;
	auto& it = resources.find(name);
	if (it != resources.end())
		res = it->second;
	else
	{
		lock.unlock();
		return false;
	}
	lock.unlock();

	if (res.data != nullptr && (res.refCount <= 1 || forceDelete))
	{
		lock.lock();
		bool success = true;

		switch (res.type) 
		{
		case Data_Type::IMAGE_1D:
			SAFE_DELETE(reinterpret_cast<const Texture1D*&>(res.data));
			break;
		case Data_Type::IMAGE_2D:
			SAFE_DELETE(reinterpret_cast<const Texture2D*&>(res.data));
			break;
		case Data_Type::IMAGE_3D:
			SAFE_DELETE(reinterpret_cast<const Texture3D*&>(res.data));
			break;
		case Data_Type::VERTEXSHADER:
			SAFE_DELETE(reinterpret_cast<const VertexShader*&>(res.data));
			break;
		case Data_Type::PIXELSHADER:
			SAFE_DELETE(reinterpret_cast<const PixelShader*&>(res.data));
			break;
		case Data_Type::GEOMETRYSHADER:
			SAFE_DELETE(reinterpret_cast<const GeometryShader*&>(res.data));
			break;
		case Data_Type::HULLSHADER:
			SAFE_DELETE(reinterpret_cast<const HullShader*&>(res.data));
			break;
		case Data_Type::DOMAINSHADER:
			SAFE_DELETE(reinterpret_cast<const DomainShader*&>(res.data));
			break;
		case Data_Type::COMPUTESHADER:
			SAFE_DELETE(reinterpret_cast<const ComputeShader*&>(res.data));
			break;
		case Data_Type::SOUND:
		case Data_Type::MUSIC:
			SAFE_DELETE(reinterpret_cast<const wiSound*&>(res.data));
			break;
		default:
			success = false;
			break;
		};

		resources.erase(name);

		lock.unlock();

		return success;
	}
	else if (res.data != nullptr)
	{
		lock.lock();
		resources[name].refCount--;
		lock.unlock();
	}
	return false;
}

bool wiResourceManager::Register(const wiHashString& name, void* resource, Data_Type newType)
{
	lock.lock();
	if (resources.find(name) == resources.end())
	{
		Resource res;
		res.data = resource;
		res.type = newType;
		res.refCount = 1;
		resources.insert(make_pair(name, res));
		lock.unlock();
		return true;
	}
	lock.unlock();

	return false;
}

bool wiResourceManager::Clear()
{
	wiRenderer::GetDevice()->WaitForGPU();

	std::vector<wiHashString> resNames;
	resNames.reserve(resources.size());
	for (auto& x : resources)
	{
		resNames.push_back(x.first);
	}
	for (auto& x : resNames)
	{
		del(x);
	}
	lock.lock();
	resources.clear();
	lock.unlock();
	return true;
}
