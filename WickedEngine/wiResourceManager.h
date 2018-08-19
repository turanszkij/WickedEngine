#pragma once
#include "CommonInclude.h"
#include "wiThreadSafeManager.h"
#include "wiGraphicsAPI.h"
#include "wiHashString.h"

#include <map>
#include <unordered_map>

class wiResourceManager : public wiThreadSafeManager
{
public:
	enum Data_Type{
		DYNAMIC,
		IMAGE,
		SOUND,MUSIC,
		VERTEXSHADER,
		PIXELSHADER,
		GEOMETRYSHADER,
		HULLSHADER,
		DOMAINSHADER,
		COMPUTESHADER,
	};

	struct Resource
	{
		void* data;
		Data_Type type;
		long refCount;

		Resource(void* newData, Data_Type newType) :data(newData), type(newType)
		{
			refCount = 1;
		};
	};
	std::unordered_map<wiHashString, Resource*> resources;

protected:
static wiResourceManager* globalResources;


public:
	wiResourceManager();
	~wiResourceManager();
	static wiResourceManager* GetGlobal();
	static wiResourceManager* GetShaderManager();

	const Resource* get(const wiHashString& name, bool IncRefCount = false);
	//specify datatype for shaders
	void* add(const wiHashString& name, Data_Type newType = Data_Type::DYNAMIC);
	bool del(const wiHashString& name, bool forceDelete = false);
	bool Register(const wiHashString& name, void* resource, Data_Type newType);
	bool CleanUp();
};

