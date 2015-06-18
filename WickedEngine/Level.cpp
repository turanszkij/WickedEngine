#include "Renderer.h"

extern Camera* cam;


extern FLOAT shBias;
extern FLOAT BlackOut, BlackWhite, InvertCol;


WorldInfo Renderer::worldInfo;
Wind Renderer::wind;

Level::Level(string name)
{
	identifier="_L";

	Renderer::Renderer();
	wind=Wind();

	stringstream ss("");
	ss<<"levels/"<<name<<"/";

	LoadFromDisk(ss.str(),name,identifier,armatures,materials,objects,objects_trans,objects_water
		,meshes,lights,spheres,worldInfo,wind,cameras
		,e_armatures,e_objects,transforms,decals);

	stringstream enviroMapFilePath("");
	enviroMapFilePath<<"levels/"<<name<<"/"<<"/env.dds";
	enviroMap = (ID3D11ShaderResourceView*)ResourceManager::add(enviroMapFilePath.str());

	playArea=Hitbox2D(XMFLOAT2(0,0),XMFLOAT2(20,20));

	stringstream colorGradingFilePath("");
	colorGradingFilePath<<"levels/"<<name<<"/"<<"/colorGrading.dds";
	colorGrading = (TextureView)ResourceManager::add(colorGradingFilePath.str());
}

void* Level::operator new(size_t size)
{
	void* result = _aligned_malloc(size,16);
	return result;
}
void Level::operator delete(void* p)
{
	if(p) _aligned_free(p);
}

void Level::CleanUp()
{
	Entity::CleanUp();


	if(enviroMap) {
		stringstream enviroMapFilePath("");
		enviroMapFilePath<<DIRECTORY<<"/env.dds";
		ResourceManager::del(enviroMapFilePath.str());
	}
	if(colorGrading) {
		stringstream colorGradingFilePath("");
		colorGradingFilePath<<DIRECTORY<<"/colorGrading.png";
		ResourceManager::del(colorGradingFilePath.str());
	}
	if(spTree_lights)
		spTree_lights->CleanUp();
	spTree_lights=nullptr;

	delete(this);
}
