#pragma once
#include "CommonInclude.h"
#include "wiSceneComponents.h"

class wiTranslator : public wiSceneComponents::Transform
{
private:
	XMFLOAT4 prevPointer;
	XMFLOAT4X4 dragStart, dragEnd;
	bool dragging;
	bool dragStarted;
	bool dragEnded;
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


	// Check if the drag started in this exact frame
	bool IsDragStarted();
	XMFLOAT4X4 GetDragStart();
	// Check if the drag ended in this exact frame
	bool IsDragEnded();
	XMFLOAT4X4 GetDragEnd();

	static wiGraphicsTypes::GPUBuffer* vertexBuffer_Axis;
	static wiGraphicsTypes::GPUBuffer* vertexBuffer_Plane;
	static wiGraphicsTypes::GPUBuffer* vertexBuffer_Origin;
	static int vertexCount_Axis;
	static int vertexCount_Plane;
	static int vertexCount_Origin;
};

