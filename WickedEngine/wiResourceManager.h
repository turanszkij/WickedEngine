#pragma once
#include "CommonInclude.h"
#include "wiThreadSafeManager.h"
#include "wiGraphicsAPI.h"

class wiSound;

class wiResourceManager : public wiThreadSafeManager
{
public:
	enum Data_Type{
		DYNAMIC,
		IMAGE,IMAGE_STAGING,
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
	typedef unordered_map<string, Resource*> container;
	container resources;

protected:
typedef map<string,Data_Type> filetypes;
static filetypes types;
static wiResourceManager* globalResources;
static void SetUp();


public:
	wiResourceManager();
	~wiResourceManager();
	static wiResourceManager* GetGlobal();
	static wiResourceManager* GetShaderManager();

	const Resource* get(const string& name, bool IncRefCount = false);
	//specify datatype for shaders
	void* add(const string& name, Data_Type newType = Data_Type::DYNAMIC
		, wiGraphicsTypes::VertexLayoutDesc* vertexLayoutDesc = nullptr, UINT elementCount = 0, wiGraphicsTypes::StreamOutDeclaration* streamOutDecl = nullptr);
	bool del(const string& name, bool forceDelete = false);
	bool CleanUp();
};

