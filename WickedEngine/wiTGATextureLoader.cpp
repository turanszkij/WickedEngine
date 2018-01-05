#include "wiTGATextureLoader.h"
#include "wiHelper.h"

#include <fstream>

using namespace std;

wiTGATextureLoader::wiTGATextureLoader() : texels(nullptr)
{
}
wiTGATextureLoader::~wiTGATextureLoader()
{
	SAFE_DELETE_ARRAY(texels);
}

void wiTGATextureLoader::load(const string& fileName) 
{
	wiHelper::DataBlob file(fileName);

	if (file.DATA == nullptr)
	{
		return;
	}

	/*Fill our header.*/
	file.read(header.idlength);
	file.read(header.colourmaptype);
	file.read(header.datatypecode);
	file.read(header.colourmaporigin);
	file.read(header.colourmaplength);
	file.read(header.colourmapdepth);
	file.read(header.x_origin);
	file.read(header.y_origin);
	file.read(header.width);
	file.read(header.height);
	file.read(header.bitsperpixel);
	file.read(header.imagedescriptor);

	/*Allocate memory*/
	texels = new uint8_t[header.width * header.height * 4];

	/*Start reading and converting our image!*/
	for (int y = 0; y < header.height; ++y)
	{
		for (int x = 0; x < header.width; ++x)
		{
			// Origin conversion:
			int _X = x;
			int _Y = header.height - 1 - y;
			int gridpos = (_X + _Y * header.width) * 4;

			/*BGRA --> RGBA*/
			file.read(texels[gridpos + 2]);
			file.read(texels[gridpos + 1]);
			file.read(texels[gridpos + 0]);
			if (header.bitsperpixel == 32)
			{
				file.read(texels[gridpos + 3]);
			}
			else if (header.bitsperpixel == 24)
			{
				texels[gridpos + 3] = 255;
			}
		}
	}
}
