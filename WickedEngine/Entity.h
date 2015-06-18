#pragma once 

#include "Renderer.h"

class Entity : public Renderer
{
protected:
	string identifier;
	vector<Armature*> e_armatures;
	vector<Object*> e_objects;

	void CleanUp(){
		Renderer::CleanUp();
		identifier.clear();
		e_armatures.clear();
		e_objects.clear();
	}
};
