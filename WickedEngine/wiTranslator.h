#pragma once
#include "CommonInclude.h"
#include "wiLoader.h"

class wiTranslator : public Transform
{
private:
	XMFLOAT4 prevPointer;
public:
	wiTranslator();
	~wiTranslator();

	void Update();


	bool enabled;

	static wiGraphicsTypes::GPUBuffer* vertexBuffer;
	static int vertexCount;
};

