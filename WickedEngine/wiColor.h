#pragma once

class wiColor
{
public:
	wiColor(unsigned char r = 0, unsigned char g = 0, unsigned char b = 0, unsigned char a = 255);
	~wiColor(){}

	float R, G, B, A;
	unsigned char r, g, b, a;
	unsigned long rgb, rgba;

	unsigned long createRGB(int r, int g, int b);
	unsigned long createRGBA(int r, int g, int b, int a);

	static wiColor fromFloat(float r = 0, float g = 0, float b = 0, float a = 1);
	static wiColor lerp(const wiColor& a, const wiColor& b, float i);

	static wiColor Red;
	static wiColor Green;
	static wiColor Blue;
	static wiColor Black;
	static wiColor White;
	static wiColor Yellow;
	static wiColor Purple;
	static wiColor Cyan;
	static wiColor Transparent;
	static wiColor Gray;
	static wiColor Ghost;
	static wiColor Booger;
};


