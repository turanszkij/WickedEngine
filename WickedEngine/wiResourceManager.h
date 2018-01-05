#pragma once
#include "CommonInclude.h"
#include "wiThreadSafeManager.h"
#include "wiGraphicsAPI.h"
#include "wiHashString.h"

#include <map>
#include <unordered_map>

class wiSound;

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
	typedef std::unordered_map<wiHashString, Resource*> container;
	container resources;

protected:
typedef std::map<std::string,Data_Type> filetypes;
static filetypes types;
static wiResourceManager* globalResources;
static void SetUp();


public:
	wiResourceManager();
	~wiResourceManager();
	static wiResourceManager* GetGlobal();
	static wiResourceManager* GetShaderManager();

	const Resource* get(const wiHashString& name, bool IncRefCount = false);
	//specify datatype for shaders
	void* add(const wiHashString& name, Data_Type newType = Data_Type::DYNAMIC
		, wiGraphicsTypes::VertexLayoutDesc* vertexLayoutDesc = nullptr, UINT elementCount = 0);
	bool del(const wiHashString& name, bool forceDelete = false);
	bool CleanUp();
};

