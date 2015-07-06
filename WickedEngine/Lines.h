#pragma once
#include "CommonInclude.h"


class Lines
{
private:
	struct Description{
		XMFLOAT4X4 transform;
		float length;
		XMFLOAT4A color;

		Description(){
			length=0;
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
	void SetUpVertices();
public:
	Lines();
	Lines(float newLen, const XMFLOAT4A& newColor, int newParentArmature, int newParentBone);
	Lines(const XMFLOAT3& a, const XMFLOAT3& b, const XMFLOAT4A& c);
	void CleanUp();
	void* operator new(size_t size){ void* result = _aligned_malloc(size,16); return result; }
	void operator delete(void* p){ if(p) _aligned_free(p); }
	void Transform(const XMFLOAT4X4& mat);

	Description desc;
	int parentArmature,parentBone;
	
	ID3D11Buffer* vertexBuffer;
};

