#pragma once
#include "CommonInclude.h"


class Cube
{
private:
	struct Description{
		XMFLOAT3 center,halfwidth;
		XMFLOAT4X4 transform;
		XMFLOAT4A color;

		Description(){
			center=XMFLOAT3(0,0,0);
			halfwidth=XMFLOAT3(1,1,1);
			color=XMFLOAT4A(1,1,1,1);
			transform=XMFLOAT4X4(
				1,0,0,0,
				0,1,0,0,
				0,0,1,0,
				0,0,0,1
				);
		};
	};
	struct Vertex
	{
		XMFLOAT3A	pos;

		Vertex(){ pos=XMFLOAT3A(0,0,0); }
		Vertex(const XMFLOAT3A& newPos){ pos=newPos; }
	};
	static void SetUpVertices();
public:
	Cube(const XMFLOAT3& center=XMFLOAT3(0,0,0), const XMFLOAT3& halfwidth=XMFLOAT3(1,1,1), const XMFLOAT4A& color = XMFLOAT4A(1,1,1,1));
	void* operator new(size_t size){ void* result = _aligned_malloc(size,16); return result; }
	void operator delete(void* p){ if(p) _aligned_free(p); }
	
	void Transform(const XMFLOAT4X4& mat);
	void Transform(const XMMATRIX& mat);

	Description desc;
	
	static ID3D11Buffer* vertexBuffer;
	static ID3D11Buffer* indexBuffer;
	static void LoadStatic();
	static void CleanUpStatic();
};

