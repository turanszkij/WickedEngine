#pragma once

class wiColor
{
public:
	wiColor(unsigned char r = 0, unsigned char g = 0, unsigned char b = 0, unsigned char a = 255);
	~wiColor(){}

	unsigned char r, g, b, a;
	unsigned long rgb, rgba;

	unsigned long createRGB(int r, int g, int b);
	unsigned long createRGBA(int r, int g, int b, int a);
};


