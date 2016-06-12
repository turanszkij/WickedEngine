#include "wiLines.h"
#include "wiRenderer.h"

using namespace wiGraphicsTypes;

Lines::Lines()
{
	desc = Description();
	desc.length=1;
	parentArmature=0;
	parentBone=0;
	SetUpVertices();
}

Lines::Lines(float newLen, const XMFLOAT4& newColor, int newParentArmature, int newParentBone)
{
	desc = Description();
	desc.length = newLen;
	desc.color = newColor;
	parentArmature = newParentArmature;
	parentBone = newParentBone;
	SetUpVertices();
}

Lines::Lines(const XMFLOAT3& a, const XMFLOAT3& b, const XMFLOAT4& c)
{
	desc = Description();
	desc.length = 0;
	desc.color = c;
	parentArmature = 0;
	parentBone = 0;

	//Vertex* verts = new Vertex[2];

	//verts[0].pos = XMFLOAT3(a.x,a.y,a.z);
	//verts[1].pos = XMFLOAT3(b.x,b.y,b.z);

	XMFLOAT4 verts[] = {
		XMFLOAT4(a.x,a.y,a.z,1), XMFLOAT4(1,1,1,1),
		XMFLOAT4(b.x,b.y,b.z,1), XMFLOAT4(1,1,1,1),
	};

	GPUBufferDesc bd;
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = USAGE_DEFAULT;
	bd.ByteWidth = sizeof(verts);
	bd.BindFlags = BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	SubresourceData InitData;
	ZeroMemory( &InitData, sizeof(InitData) );
	InitData.pSysMem = verts;
	wiRenderer::GetDevice()->CreateBuffer( &bd, &InitData, &vertexBuffer );

	//if(verts){
	//	delete[](verts);
	//	verts=NULL;
	//}
}
Lines::~Lines()
{
	CleanUp();
}

void Lines::CleanUp()
{
	
}

void Lines::SetUpVertices()
{
	//Vertex* verts = new Vertex[2];

	//verts[0].pos = XMFLOAT3(0,0,0);
	//verts[1].pos = XMFLOAT3(0,0,desc.length);

	//GPUBufferDesc bd;
	//ZeroMemory( &bd, sizeof(bd) );
	//bd.Usage = USAGE_DEFAULT;
	//bd.ByteWidth = sizeof( Vertex ) * 2;
	//bd.BindFlags = BIND_VERTEX_BUFFER;
	//bd.CPUAccessFlags = 0;
	//SubresourceData InitData;
	//ZeroMemory( &InitData, sizeof(InitData) );
	//InitData.pSysMem = verts;
	//wiRenderer::GetDevice()->CreateBuffer( &bd, &InitData, &vertexBuffer );

	//if(verts){
	//	delete[](verts);
	//	verts=NULL;
	//}





	//Vertex* verts = new Vertex[2];

	//verts[0].pos = XMFLOAT3(a.x,a.y,a.z);
	//verts[1].pos = XMFLOAT3(b.x,b.y,b.z);

	XMFLOAT4 verts[] = {
		XMFLOAT4(0,0,0,1), XMFLOAT4(1,1,1,1),
		XMFLOAT4(0,0,desc.length,1), XMFLOAT4(1,1,1,1),
	};

	GPUBufferDesc bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = USAGE_DEFAULT;
	bd.ByteWidth = sizeof(verts);
	bd.BindFlags = BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	SubresourceData InitData;
	ZeroMemory(&InitData, sizeof(InitData));
	InitData.pSysMem = verts;
	wiRenderer::GetDevice()->CreateBuffer(&bd, &InitData, &vertexBuffer);

	//if(verts){
	//	delete[](verts);
	//	verts=NULL;
	//}
}

void Lines::Transform(const XMFLOAT4X4& mat)
{
	desc.transform=mat;
}