#include "ResourceManager.h"


ResourceManager::container ResourceManager::resources;
ResourceManager::filetypes ResourceManager::types;
mutex ResourceManager::MUTEX;

void ResourceManager::SetUp()
{
	types.insert( pair<string,Data_Type>("jpg",IMAGE) );
	types.insert( pair<string,Data_Type>("JPG",IMAGE) );
	types.insert( pair<string,Data_Type>("png",IMAGE) );
	types.insert( pair<string,Data_Type>("PNG",IMAGE) );
	types.insert( pair<string,Data_Type>("dds",IMAGE) );
	types.insert( pair<string,Data_Type>("wav",SOUND) );
}

const ResourceManager::Resource* ResourceManager::get(const string& name)
{
	container::iterator it = resources.find(name);
	if(it!=resources.end())
		return it->second;
	else return nullptr;
}

void* ResourceManager::add(const string& name, Data_Type newType
	, Renderer::VertexLayoutDesc* vertexLayoutDesc, UINT elementCount, D3D11_SO_DECLARATION_ENTRY* streamOutDecl)
{
	const Resource* res = get(name);
	if(!res){
		string ext = name.substr(name.length()-3,name.length());
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

		MUTEX.lock();

		switch(type){
		case Data_Type::IMAGE:
		{
			Renderer::TextureView image=nullptr;
			if(
					!ext.compare("jpg")
				|| !ext.compare("JPG")
				|| !ext.compare("png")
				|| !ext.compare("PNG")
				)
			{
				Renderer::graphicsMutex.lock();
				CreateWICTextureFromFile(true,Renderer::graphicsDevice,Renderer::immediateContext,(wchar_t*)(wstring(name.begin(),name.end()).c_str()),nullptr,&image);
				Renderer::graphicsMutex.unlock();
			}
			else if(!ext.compare("dds")){
				CreateDDSTextureFromFile(Renderer::graphicsDevice,(wchar_t*)(wstring(name.begin(),name.end()).c_str()),nullptr,&image);
			}

			if(image)
				success=image;
		}
		break;
		case Data_Type::IMAGE_STAGING:
		{
			Renderer::APIResource image=nullptr;
			if(!ext.compare("dds")){
				CreateDDSTextureFromFileEx(Renderer::graphicsDevice,(wchar_t*)(wstring(name.begin(),name.end()).c_str()),0
					,D3D11_USAGE_STAGING,0,D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE,0,false
					,&image,nullptr);
			}

			if(image)
				success=image;
		}
		break;
		case Data_Type::SOUND:
		{
			success = new SoundEffect(name);
		}
		break;
		case Data_Type::MUSIC:
		{
			success = new Music(name);
		}
		break;
		case Data_Type::VERTEXSHADER:
		{
			BYTE* buffer;
			size_t bufferSize;
			if (WickedHelper::readByteData(name, &buffer, bufferSize)){
				Renderer::VertexShader shader = nullptr;
				Renderer::graphicsDevice->CreateVertexShader(buffer, bufferSize, NULL, &shader);
				if (vertexLayoutDesc != nullptr && elementCount > 0){
					Renderer::VertexShaderInfo* vertexShaderInfo = new Renderer::VertexShaderInfo();
					vertexShaderInfo->vertexShader = shader;
					Renderer::graphicsDevice->CreateInputLayout(vertexLayoutDesc, elementCount, buffer, bufferSize, &vertexShaderInfo->vertexLayout);
					if (vertexShaderInfo->vertexShader != nullptr && vertexShaderInfo->vertexLayout != nullptr){
						success = vertexShaderInfo;
					}
				}
				else{
					success = shader;
				}
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
			if (WickedHelper::readByteData(name, &buffer, bufferSize)){
				Renderer::PixelShader shader = nullptr;
				Renderer::graphicsDevice->CreatePixelShader(buffer, bufferSize, nullptr, &shader);
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
			if (WickedHelper::readByteData(name, &buffer, bufferSize)){
				Renderer::GeometryShader shader = nullptr;
				Renderer::graphicsDevice->CreateGeometryShader(buffer, bufferSize, nullptr, &shader);
				if (streamOutDecl != nullptr && elementCount > 0){
					Renderer::graphicsDevice->CreateGeometryShaderWithStreamOutput(buffer, bufferSize, streamOutDecl,
						elementCount, NULL, 0, shader ? 0 : D3D11_SO_NO_RASTERIZED_STREAM, NULL, &shader);
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
			if (WickedHelper::readByteData(name, &buffer, bufferSize)){
				Renderer::HullShader shader = nullptr;
				Renderer::graphicsDevice->CreateHullShader(buffer, bufferSize, nullptr, &shader);
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
			if (WickedHelper::readByteData(name, &buffer, bufferSize)){
				Renderer::DomainShader shader = nullptr;
				Renderer::graphicsDevice->CreateDomainShader(buffer, bufferSize, nullptr, &shader);
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
			//TODO
			success = nullptr;
		}
		break;
		default:
			success=nullptr;
			break;
		};

		if(success)
			resources.insert( pair<string,Resource*>(name,new Resource(success,type)) );

		MUTEX.unlock();

		return success;
	}
	return res->data;
}

bool ResourceManager::del(const string& name)
{
	const Resource* res = get(name);

	if(res){
		MUTEX.lock();
		bool success = true;

		if(res->data)
			switch(res->type){
			case Data_Type::IMAGE:
				((Renderer::TextureView)res->data)->Release();
				break;
			case Data_Type::SOUND:
				delete ((SoundResource)res->data);
				break;
			default:
				success=false;
				break;
			};

		resources.erase(name);

		MUTEX.unlock();

		return success;
	}
	return false;
}

bool ResourceManager::CleanUp()
{
	/*vector<string> names;
	names.reserve(resources.size());
	for(container::iterator it=resources.begin();it!=resources.end();++it)
		names.push_back(it->first);
	for(const string& s:names)
		del(s);
	names.clear();*/
	resources.clear();
	types.clear();
	return true;
}