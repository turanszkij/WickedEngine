#pragma once
#include "Hitbox2D.h"

class Level:public Entity{
public:
	Level(string name);
	void CleanUp();
	
	void* operator new(size_t);
	void operator delete(void*);

	Hitbox2D playArea;
};
