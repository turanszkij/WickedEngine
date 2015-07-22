#pragma once
#include "CommonInclude.h"


class Lines
{
private:
	struct Description{
		XMFLOAT4X4 transform;
		float length;
		XMFLOAT4 color;

		Description(){
			length=0;
			color=XMFLOAT4(1,1,1,1);
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
		XMFLOAT3	pos;
		float padding;

		Vertex(){ pos=XMFLOAT3(0,0,0); }
		Vertex(const XMFLOAT3& newPos){ pos=newPos; }
	};
	void SetUpVertices();
public:
	Lines();
	Lines(float newLen, const XMFLOAT4& newColor, int newParentArmature, int newParentBone);
	Lines(const XMFLOAT3& a, const XMFLOAT3& b, const XMFLOAT4& c);
	void CleanUp();
	void Transform(const XMFLOAT4X4& mat);

	Description desc;
	int parentArmature,parentBone;
	
	ID3D11Buffer* vertexBuffer;

	ALIGN_16
};

