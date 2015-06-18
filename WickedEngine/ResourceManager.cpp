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

void* ResourceManager::add(const string& name, Data_Type newType)
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