#pragma once
#include "CommonInclude.h"
#include "wiGraphicsAPI.h"

class Cube
{
private:
	struct Description{
		XMFLOAT3 center,halfwidth;
		XMFLOAT4X4 transform;
		XMFLOAT4 color;

		Description(){
			center=XMFLOAT3(0,0,0);
			halfwidth=XMFLOAT3(1,1,1);
			color=XMFLOAT4(1,1,1,1);
			transform=XMFLOAT4X4(
				1,0,0,0,
				0,1,0,0,
				0,0,1,0,
				0,0,0,1
				);
		};
	};
	static void SetUpVertices();
public:
	Cube(const XMFLOAT3& center=XMFLOAT3(0,0,0), const XMFLOAT3& halfwidth=XMFLOAT3(1,1,1), const XMFLOAT4& color = XMFLOAT4(1,1,1,1));

	void Transform(const XMFLOAT4X4& mat);
	void Transform(const XMMATRIX& mat);

	Description desc;
	
	static wiGraphicsTypes::GPUBuffer vertexBuffer;
	static wiGraphicsTypes::GPUBuffer indexBuffer;
	static void LoadStatic();
	static void CleanUpStatic();

	ALIGN_16
};

