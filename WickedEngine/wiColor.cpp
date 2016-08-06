#include "wiColor.h"
#include "wiMath.h"

wiColor::wiColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a) :r(r), g(g), b(b), a(a)
{
	rgb = createRGB(r, g, b);
	rgba = createRGBA(r, g, b, a);
	R = r / 255.f;
	G = g / 255.f;
	B = b / 255.f;
	A = a / 255.f;
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

wiColor wiColor::fromFloat(float r, float g, float b, float a)
{
	return wiColor((unsigned char)(r * 255), (unsigned char)(g * 255), (unsigned char)(b * 255), (unsigned char)(a * 255));
}
wiColor wiColor::lerp(const wiColor& a, const wiColor& b, float i)
{
	XMFLOAT4& retF = wiMath::Lerp(XMFLOAT4(a.R, a.G, a.B, a.A), XMFLOAT4(b.R, b.G, b.B, b.A), i);
	return wiColor::fromFloat(retF.x, retF.y, retF.z, retF.w);
}

wiColor wiColor::Red				= wiColor(255, 0, 0, 255);
wiColor wiColor::Green				= wiColor(0, 255, 0, 255);
wiColor wiColor::Blue				= wiColor(0, 0, 255, 255);
wiColor wiColor::Black				= wiColor(0, 0, 0, 255);
wiColor wiColor::White				= wiColor(255, 255, 255, 255);
wiColor wiColor::Yellow				= wiColor(255, 255, 0, 255);
wiColor wiColor::Purple				= wiColor(255, 0, 255, 255);
wiColor wiColor::Cyan				= wiColor(0, 255, 255, 255);
wiColor wiColor::Transparent		= wiColor(0, 0, 0, 0);
wiColor wiColor::Gray				= wiColor(127, 127, 127, 255);
wiColor wiColor::Ghost				= wiColor(127, 127, 127, 127);
wiColor wiColor::Booger				= wiColor(127, 127, 127, 200);

