#pragma once
#include "CommonInclude.h"

#include <string>
#include <intsafe.h>

struct TGAHeader{
	char  idlength;
	char  colourmaptype;
	char  datatypecode;
	short int colourmaporigin;
	short int colourmaplength;
	char  colourmapdepth;
	short int x_origin;
	short int y_origin;
	short width;
	short height;
	char bitsperpixel;
	char imagedescriptor;
};

class wiTGATextureLoader
{
public:
	TGAHeader header;
	uint8_t *texels;

	wiTGATextureLoader();
	~wiTGATextureLoader();

	void load(const std::string& fname);
};
