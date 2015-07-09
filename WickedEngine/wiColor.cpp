#include "wiColor.h"

wiColor::wiColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a) :r(r), g(g), b(b), a(a)
{
	rgb = createRGB(r, g, b);
	rgba = createRGBA(r, g, b, a);
}

unsigned long wiColor::createRGB(int r, int g, int b)
{
	return ((r & 0xff) << 16) + ((g & 0xff) << 8) + (b & 0xff);
}
unsigned long wiColor::createRGBA(int r, int g, int b, int a)
{
	return ((r & 0xff) << 24) + ((g & 0xff) << 16) + ((b & 0xff) << 8)
		+ (a & 0xff);
}

