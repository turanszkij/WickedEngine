#pragma once
#include "WickedEngine.h"

class ResourceManager
{
public:
	typedef Sound* SoundResource;
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
private:
struct Resource
{
	void* data;
	Data_Type type;

	Resource(void* newData, Data_Type newType):data(newData),type(newType){};
};
typedef map<string,Data_Type> filetypes;
static filetypes types;
typedef unordered_map<string,Resource*> container;
static container resources;
static mutex MUTEX;
public:
	static void SetUp();

	static const Resource* get(const string& name);
	//please specify datatype for shaders
	static void* add(const string& name, Data_Type newType = Data_Type::DYNAMIC
		, D3D11_INPUT_ELEMENT_DESC* vertexLayoutDesc = nullptr, UINT elementCount = 0, D3D11_SO_DECLARATION_ENTRY* streamOutDecl = nullptr);
	static bool del(const string& name);
	static bool CleanUp();
};

