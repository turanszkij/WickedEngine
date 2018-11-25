#include "wiResourceManager.h"
#include "wiRenderer.h"
#include "wiSound.h"
#include "wiHelper.h"
#include "wiTextureHelper.h"

#include "Utility/stb_image.h"
#include "Utility/nv_dds.h"

using namespace std;
using namespace wiGraphicsTypes;

static const std::unordered_map<std::string, wiResourceManager::Data_Type> types = {
	make_pair("JPG", wiResourceManager::IMAGE),
	make_pair("PNG", wiResourceManager::IMAGE),
	make_pair("DDS", wiResourceManager::IMAGE),
	make_pair("TGA", wiResourceManager::IMAGE),
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


const wiResourceManager::Resource* wiResourceManager::get(const wiHashString& name, bool incRefCount)
{
	LOCK();
	auto& it = resources.find(name);
	if (it != resources.end())
	{
		if(incRefCount)
			it->second->refCount++;
		UNLOCK();
		return it->second;
	}

	UNLOCK();
	return nullptr;
}

void* wiResourceManager::add(const wiHashString& name, Data_Type newType)
{
	const Resource* res = get(name,true);
	if(!res)
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

		switch(type){
		case Data_Type::IMAGE:
		{
			Texture2D* image = nullptr;

			if (!ext.compare(std::string("DDS")))
			{
				// Load dds

				nv_dds::CDDSImage img;
				img.load(nameStr, false);

				if (img.is_valid())
				{
					TextureDesc desc;
					desc.ArraySize = 1;
					desc.BindFlags = BIND_SHADER_RESOURCE;
					desc.CPUAccessFlags = 0;
					desc.Height = img.get_height();
					desc.Width = img.get_width();
					desc.MipLevels = 1;
					desc.MiscFlags = 0;
					desc.Usage = USAGE_IMMUTABLE;

					switch (img.get_format())
					{
					case GL_RGB:
					case GL_RGBA:
						desc.Format = FORMAT_R8G8B8A8_UNORM;
						break;
					case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
						desc.Format = FORMAT_BC1_UNORM;
						break;
					case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
						desc.Format = FORMAT_BC1_UNORM;
						break;
					case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
						desc.Format = FORMAT_BC2_UNORM;
						break;
					case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
						desc.Format = FORMAT_BC3_UNORM;
						break;
					default:
						desc.Format = FORMAT_B8G8R8A8_UNORM;
						break;
					}

					std::vector<SubresourceData> InitData;

					if (img.is_volume())
					{
						assert(0); // TODO
					}

					if (img.is_cubemap())
					{
						desc.ArraySize = 6;
						desc.MiscFlags = RESOURCE_MISC_TEXTURECUBE;

						desc.MipLevels = 1 + img.get_num_mipmaps();
						InitData.resize(desc.MipLevels * 6);

						for (UINT dir = 0; dir < 6; ++dir)
						{
							const nv_dds::CTexture& face = img.get_cubemap_face(dir);

							const UINT idx = dir * desc.MipLevels;
							InitData[idx].pSysMem = face.operator uint8_t *();
							InitData[idx].SysMemPitch = static_cast<UINT>(face.get_width() * 4);

							if (face.get_num_mipmaps() > 0)
							{
								for (UINT i = 0; i < face.get_num_mipmaps(); ++i)
								{
									const nv_dds::CSurface& surf = face.get_mipmap(i);

									InitData[idx + i + 1].pSysMem = surf.operator uint8_t *();
									InitData[idx + i + 1].SysMemPitch = InitData[idx + i].SysMemPitch / 2;
								}
							}
						}
					}
					else
					{
						desc.MipLevels = 1 + img.get_num_mipmaps();
						InitData.resize(desc.MipLevels);

						InitData[0].pSysMem = img.operator uint8_t *();
						InitData[0].SysMemPitch = static_cast<UINT>(img.get_size() / img.get_height() * 4 /* img.get_components()*/); // todo: review + vulkan api slightly different

						if (img.get_num_mipmaps() > 0)
						{
							for (UINT i = 0; i < img.get_num_mipmaps(); ++i)
							{
								const nv_dds::CSurface& surf = img.get_mipmap(i);

								InitData[i + 1].pSysMem = surf.operator uint8_t *();
								//InitData[i + 1].SysMemPitch = static_cast<UINT>(surf.get_size() / surf.get_height() * img.get_components());
								InitData[i + 1].SysMemPitch = InitData[i].SysMemPitch / 2;
							}
						}
					}

					HRESULT hr = wiRenderer::GetDevice()->CreateTexture2D(&desc, InitData.data(), &image);
					assert(SUCCEEDED(hr));
					wiRenderer::GetDevice()->SetName(image, nameStr);

				}

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
					desc.MipLevels = (UINT)log2(max(width, height));
					desc.MiscFlags = 0;
					desc.Usage = USAGE_DEFAULT;

					UINT mipwidth = width;
					SubresourceData* InitData = new SubresourceData[desc.MipLevels];
					for (UINT mip = 0; mip < desc.MipLevels; ++mip)
					{
						InitData[mip].pSysMem = rgb;
						InitData[mip].SysMemPitch = static_cast<UINT>(mipwidth * channelCount);
						mipwidth = max(1, mipwidth / 2);
					}

					image = new Texture2D;
					image->RequestIndependentShaderResourcesForMIPs(true);
					image->RequestIndependentUnorderedAccessResourcesForMIPs(true);
					HRESULT hr = wiRenderer::GetDevice()->CreateTexture2D(&desc, InitData, &image);
					assert(SUCCEEDED(hr));
					wiRenderer::GetDevice()->SetName(image, nameStr);

					if (image != nullptr && image->GetDesc().MipLevels > 1)
					{
						wiRenderer::AddDeferredMIPGen(image);
					}
				}

				stbi_image_free(rgb);
			}

			success = image;
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
			else{
				success = nullptr;
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
			else{
				success = nullptr;
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
			else{
				success = nullptr;
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
			else{
				success = nullptr;
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
			else{
				success = nullptr;
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
			else {
				success = nullptr;
			}
		}
		break;
		default:
			success=nullptr;
			break;
		};

		if (success)
		{
			LOCK();
			resources.insert(pair<wiHashString, Resource*>(name, new Resource(success, type)));
			UNLOCK();
		}

		return success;
	}

	return res->data;
}

bool wiResourceManager::del(const wiHashString& name, bool forceDelete)
{
	LOCK();
	Resource* res = nullptr;
	auto& it = resources.find(name);
	if (it != resources.end())
		res = it->second;
	else
	{
		UNLOCK();
		return false;
	}
	UNLOCK();

	if(res && (res->refCount<=1 || forceDelete))
	{
		LOCK();
		bool success = true;

		if(res->data)
			switch(res->type){
			case Data_Type::IMAGE:
				SAFE_DELETE(reinterpret_cast<Texture2D*&>(res->data));
				break;
			case Data_Type::VERTEXSHADER:
				SAFE_DELETE(reinterpret_cast<VertexShader*&>(res->data));
				break;
			case Data_Type::PIXELSHADER:
				SAFE_DELETE(reinterpret_cast<PixelShader*&>(res->data));
				break;
			case Data_Type::GEOMETRYSHADER:
				SAFE_DELETE(reinterpret_cast<GeometryShader*&>(res->data));
				break;
			case Data_Type::HULLSHADER:
				SAFE_DELETE(reinterpret_cast<HullShader*&>(res->data));
				break;
			case Data_Type::DOMAINSHADER:
				SAFE_DELETE(reinterpret_cast<DomainShader*&>(res->data));
				break;
			case Data_Type::COMPUTESHADER:
				SAFE_DELETE(reinterpret_cast<ComputeShader*&>(res->data));
				break;
			case Data_Type::SOUND:
			case Data_Type::MUSIC:
				SAFE_DELETE(reinterpret_cast<wiSound*&>(res->data));
				break;
			default:
				success=false;
				break;
			};

		delete res;
		resources.erase(name);

		UNLOCK();

		return success;
	}
	else if (res)
	{
		res->refCount--;
	}
	return false;
}

bool wiResourceManager::Register(const wiHashString& name, void* resource, Data_Type newType)
{
	LOCK();
	if (resources.find(name) == resources.end())
	{
		resources.insert(make_pair(name, new Resource(resource, newType)));
		UNLOCK();
		return true;
	}
	UNLOCK();

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
	LOCK();
	resources.clear();
	UNLOCK();
	return true;
}
