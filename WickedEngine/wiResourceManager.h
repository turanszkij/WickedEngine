#pragma once
#include "CommonInclude.h"
#include "wiGraphicsDevice.h"
#include "wiHashString.h"

#include <mutex>
#include <unordered_map>

class wiResourceManager
{
private:
	std::mutex lock;
public:
	enum Data_Type{
		DYNAMIC,
		IMAGE_1D,
		IMAGE_2D,
		IMAGE_3D,
		SOUND,MUSIC,
		VERTEXSHADER,
		PIXELSHADER,
		GEOMETRYSHADER,
		HULLSHADER,
		DOMAINSHADER,
		COMPUTESHADER,
		EMPTY,
	};

	struct Resource
	{
		const void* data = nullptr;
		Data_Type type = EMPTY;
		long refCount = 0;
	};
	std::unordered_map<wiHashString, Resource> resources;


public:
	~wiResourceManager() { Clear(); }
	static wiResourceManager& GetGlobal();
	static wiResourceManager& GetShaderManager();

	Resource get(const wiHashString& name, bool IncRefCount = false);
	//specify datatype for shaders
	const void* add(const wiHashString& name, Data_Type newType = Data_Type::DYNAMIC);
	bool del(const wiHashString& name, bool forceDelete = false);
	bool Register(const wiHashString& name, void* resource, Data_Type newType);
	bool Clear();
};

