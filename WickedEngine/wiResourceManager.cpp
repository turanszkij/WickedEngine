#include "wiResourceManager.h"
#include "wiRenderer.h"
#include "wiSound.h"
#include "wiHelper.h"
#include "wiTGATextureLoader.h"
#include "wiTextureHelper.h"

#include "Utility/stb_image.h"
#include "Utility/nv_dds.h"

using namespace std;
using namespace wiGraphicsTypes;

wiResourceManager::filetypes wiResourceManager::types;
wiResourceManager* wiResourceManager::globalResources = nullptr;

wiResourceManager::wiResourceManager():wiThreadSafeManager()
{
}
wiResourceManager::~wiResourceManager()
{
	CleanUp();
}
wiResourceManager* wiResourceManager::GetGlobal()
{
	if (globalResources == nullptr)
	{
		LOCK_STATIC();
		if (globalResources == nullptr)
		{
			globalResources = new wiResourceManager();
		}
		UNLOCK_STATIC();
	}
	return globalResources;
}
wiResourceManager* wiResourceManager::GetShaderManager()
{
	static wiResourceManager* shaderManager = new wiResourceManager;
	return shaderManager;
}

void wiResourceManager::SetUp()
{
	types.clear();
	types.insert(pair<string, Data_Type>("JPG", IMAGE));
	types.insert(pair<string, Data_Type>("PNG", IMAGE));
	types.insert(pair<string, Data_Type>("DDS", IMAGE));
	types.insert(pair<string, Data_Type>("TGA", IMAGE));
	types.insert(pair<string, Data_Type>("WAV", SOUND));
}

const wiResourceManager::Resource* wiResourceManager::get(const wiHashString& name, bool incRefCount)
{
	LOCK();
	container::iterator it = resources.find(name);
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
	if (types.empty())
		SetUp();

	const Resource* res = get(name,true);
	if(!res)
	{
		string nameStr = name.GetString();
		string ext = wiHelper::toUpper(nameStr.substr(nameStr.length() - 3, nameStr.length()));
		Data_Type type;

		// dynamic type selection:
		if(newType==Data_Type::DYNAMIC){
			filetypes::iterator it = types.find(ext);
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
						desc.Format = FORMAT_R8G8B8A8_UNORM;
						break;
					}

					std::vector<SubresourceData> InitData;

					if (img.is_cubemap())
					{
						assert(0); // TODO
					}
					if (img.is_volume())
					{
						assert(0); // TODO
					}

					desc.MipLevels = 1 + img.get_num_mipmaps();
					InitData.resize(desc.MipLevels);

					InitData[0].pSysMem = static_cast<uint8_t*>(img); // call operator
					InitData[0].SysMemPitch = static_cast<UINT>(img.get_size() / img.get_height() * 4 /* img.get_components()*/); // todo: review + vulkan api slightly different

					if (img.get_num_mipmaps() > 0)
					{
						for (UINT i = 0; i < img.get_num_mipmaps(); ++i)
						{
							const nv_dds::CSurface& surf = img.get_mipmap(i);

							InitData[i + 1].pSysMem = static_cast<uint8_t*>(surf); // call operator
							//InitData[i + 1].SysMemPitch = static_cast<UINT>(surf.get_size() / surf.get_height() * img.get_components());
							InitData[i + 1].SysMemPitch = InitData[i].SysMemPitch / 2;
						}
					}

					wiRenderer::GetDevice()->CreateTexture2D(&desc, InitData.data(), &image);

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
					wiRenderer::GetDevice()->CreateTexture2D(&desc, InitData, &image);
				}

				stbi_image_free(rgb);

				if (image != nullptr)
				{
					wiRenderer::AddDeferredMIPGen(image);
				}
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
			BYTE* buffer;
			size_t bufferSize;
			if (wiHelper::readByteData(nameStr, &buffer, bufferSize)) {
				VertexShader* shader = new VertexShader;
				wiRenderer::GetDevice()->CreateVertexShader(buffer, bufferSize, shader);
				delete[] buffer;
				success = shader;
			}
			else{
				success = nullptr;
			}
		}
		break;
		case Data_Type::PIXELSHADER:
		{
			BYTE* buffer;
			size_t bufferSize;
			if (wiHelper::readByteData(nameStr, &buffer, bufferSize)){
				PixelShader* shader = new PixelShader;
				wiRenderer::GetDevice()->CreatePixelShader(buffer, bufferSize, shader);
				delete[] buffer;
				success = shader;
			}
			else{
				success = nullptr;
			}
		}
		break;
		case Data_Type::GEOMETRYSHADER:
		{
			BYTE* buffer;
			size_t bufferSize;
			if (wiHelper::readByteData(nameStr, &buffer, bufferSize)){
				GeometryShader* shader = new GeometryShader;
				wiRenderer::GetDevice()->CreateGeometryShader(buffer, bufferSize, shader);
				delete[] buffer;
				success = shader;
			}
			else{
				success = nullptr;
			}
		}
		break;
		case Data_Type::HULLSHADER:
		{
			BYTE* buffer;
			size_t bufferSize;
			if (wiHelper::readByteData(nameStr, &buffer, bufferSize)){
				HullShader* shader = new HullShader;
				wiRenderer::GetDevice()->CreateHullShader(buffer, bufferSize, shader);
				delete[] buffer;
				success = shader;
			}
			else{
				success = nullptr;
			}
		}
		break;
		case Data_Type::DOMAINSHADER:
		{
			BYTE* buffer;
			size_t bufferSize;
			if (wiHelper::readByteData(nameStr, &buffer, bufferSize)){
				DomainShader* shader = new DomainShader;
				wiRenderer::GetDevice()->CreateDomainShader(buffer, bufferSize, shader);
				delete[] buffer;
				success = shader;
			}
			else{
				success = nullptr;
			}
		}
		break;
		case Data_Type::COMPUTESHADER:
		{
			BYTE* buffer;
			size_t bufferSize;
			if (wiHelper::readByteData(nameStr, &buffer, bufferSize)) {
				ComputeShader* shader = new ComputeShader;
				wiRenderer::GetDevice()->CreateComputeShader(buffer, bufferSize, shader);
				delete[] buffer;
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
	container::iterator it = resources.find(name);
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

bool wiResourceManager::CleanUp()
{
	wiRenderer::GetDevice()->WaitForGPU();

	std::vector<wiHashString>resNames(0);
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
