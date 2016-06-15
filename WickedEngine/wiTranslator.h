#pragma once
#include "CommonInclude.h"
#include "wiLoader.h"

class wiTranslator : public Transform
{
private:
	XMFLOAT4 prevPointer;
	XMFLOAT3 dragStart;
	bool dragging;
public:
	wiTranslator();
	~wiTranslator();

	void Update();

	bool enabled;

	enum TRANSLATOR_STATE
	{
		TRANSLATOR_IDLE,
		TRANSLATOR_X,
		TRANSLATOR_Y,
		TRANSLATOR_Z,
		TRANSLATOR_XY,
		TRANSLATOR_XZ,
		TRANSLATOR_YZ,
		TRANSLATOR_XYZ,
	} state;

	float dist;

	bool isTranslator, isScalator, isRotator;

	static wiGraphicsTypes::GPUBuffer* vertexBuffer_Axis;
	static wiGraphicsTypes::GPUBuffer* vertexBuffer_Plane;
	static wiGraphicsTypes::GPUBuffer* vertexBuffer_Origin;
	static int vertexCount_Axis;
	static int vertexCount_Plane;
	static int vertexCount_Origin;
};

