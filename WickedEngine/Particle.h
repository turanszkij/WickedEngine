#pragma once
#include "WickedEngine.h"
#define MAX_PARTICLES 1000


struct SkinnedVertex;
struct Mesh;
struct Object;

class Particle
{
protected:
	Particle(){};

public:
	
	static bool wireRender;
	static void ChangeRasterizer(){wireRender=!wireRender;}
};

#include "EmittedParticle.h"
#include "HairParticle.h"

