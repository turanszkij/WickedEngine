#pragma once
#include "CommonInclude.h"

class wiWaterPlane
{
public:
	float x, y, z, w;

	wiWaterPlane(float x = 0, float y = 1, float z = 0, float w = 0) :x(x), y(y), z(z), w(w){}

	const XMFLOAT4 getXMFLOAT4() const { return XMFLOAT4(x, y, z, w); }
};

