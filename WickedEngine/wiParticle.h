#pragma once
#include "CommonInclude.h"
#include "wiGraphicsAPI.h"
#define MAX_PARTICLES 10000


struct SkinnedVertex;
struct Mesh;
struct Object;

class wiParticle
{
protected:
	wiParticle(){};

public:
	
	static bool wireRender;
	static void ChangeRasterizer(){wireRender=!wireRender;}
};


