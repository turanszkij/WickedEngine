#include "wiResourceManager.h"
#include "wiRenderer.h"
#include "wiSound.h"
#include "wiHelper.h"

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

void* wiResourceManager::add(const wiHashString& name, Data_Type newType
	, VertexLayoutDesc* vertexLayoutDesc, UINT elementCount, StreamOutDeclaration* streamOutDecl)
{
	if (types.empty())
		SetUp();

	const Resource* res = get(name,true);
	if(!res)
	{
		string nameStr = name.GetString();
		string ext = wiHelper::toUpper(nameStr.substr(nameStr.length() - 3, nameStr.length()));
		Data_Type type;
#pragma region dynamic type selection
		if(newType==Data_Type::DYNAMIC){
			filetypes::iterator it = types.find(ext);
			if(it!=types.end())
				type = it->second;
			else 
				return nullptr;
		}
		else 
			type = newType;
#pragma endregion
		void* success = nullptr;

		LOCK();

		switch(type){
		case Data_Type::IMAGE:
		{
			Texture2D* image=nullptr;
			{
				wiRenderer::GetDevice()->CreateTextureFromFile(name.GetString().c_str(), &image, true, GRAPHICSTHREAD_IMMEDIATE);
			}

			if(image)
				success=image;
		}
		break;
		case Data_Type::IMAGE_STAGING:
		{
			wiHelper::messageBox("IMAGE_STAGING texture loading not implemented in ResourceManager!", "Warning!");
			success = nullptr;
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
			if (wiHelper::readByteData(name.GetString(), &buffer, bufferSize)){
				VertexShaderInfo* vertexShaderInfo = new VertexShaderInfo;
				vertexShaderInfo->vertexShader = new VertexShader;
				vertexShaderInfo->vertexLayout = new VertexLayout;
				wiRenderer::GetDevice()->CreateVertexShader(buffer, bufferSize, NULL, vertexShaderInfo->vertexShader);
				if (vertexLayoutDesc != nullptr && elementCount > 0){
					wiRenderer::GetDevice()->CreateInputLayout(vertexLayoutDesc, elementCount, buffer, bufferSize, vertexShaderInfo->vertexLayout);
				}
				success = vertexShaderInfo;
				delete[] buffer;
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
				wiRenderer::GetDevice()->CreatePixelShader(buffer, bufferSize, nullptr, shader);
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
				wiRenderer::GetDevice()->CreateGeometryShader(buffer, bufferSize, nullptr, shader);
				if (streamOutDecl != nullptr && elementCount > 0){
					wiRenderer::GetDevice()->CreateGeometryShaderWithStreamOutput(buffer, bufferSize, streamOutDecl,
						elementCount, NULL, 0, SO_NO_RASTERIZED_STREAM, NULL, shader);
				}
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
				wiRenderer::GetDevice()->CreateHullShader(buffer, bufferSize, nullptr, shader);
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
				wiRenderer::GetDevice()->CreateDomainShader(buffer, bufferSize, nullptr, shader);
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
				wiRenderer::GetDevice()->CreateComputeShader(buffer, bufferSize, nullptr, shader);
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

		if(success)
			resources.insert( pair<wiHashString,Resource*>(name,new Resource(success,type)) );

		UNLOCK();

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
			case Data_Type::IMAGE_STAGING:
				SAFE_DELETE(reinterpret_cast<Texture2D*&>(res->data));
				break;
			case Data_Type::VERTEXSHADER:
				SAFE_DELETE(reinterpret_cast<VertexShaderInfo*&>(res->data));
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
