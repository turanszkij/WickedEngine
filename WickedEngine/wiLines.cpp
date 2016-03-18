#include "wiLines.h"
#include "wiRenderer.h"

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

	Vertex* verts = new Vertex[2];

	verts[0].pos = XMFLOAT3(a.x,a.y,a.z);
	verts[1].pos = XMFLOAT3(b.x,b.y,b.z);

	BufferDesc bd;
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = USAGE_DEFAULT;
	bd.ByteWidth = sizeof( Vertex ) * 2;
	bd.BindFlags = BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	SubresourceData InitData;
	ZeroMemory( &InitData, sizeof(InitData) );
	InitData.pSysMem = verts;
	wiRenderer::graphicsDevice->CreateBuffer( &bd, &InitData, &vertexBuffer );

	if(verts){
		delete[](verts);
		verts=NULL;
	}
}
Lines::~Lines()
{
	CleanUp();
}

void Lines::CleanUp()
{
	if(vertexBuffer){
		vertexBuffer->Release();
		vertexBuffer = NULL;
	}
}

void Lines::SetUpVertices()
{
	Vertex* verts = new Vertex[2];

	verts[0].pos = XMFLOAT3(0,0,0);
	verts[1].pos = XMFLOAT3(0,0,desc.length);

	BufferDesc bd;
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = USAGE_DEFAULT;
	bd.ByteWidth = sizeof( Vertex ) * 2;
	bd.BindFlags = BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	SubresourceData InitData;
	ZeroMemory( &InitData, sizeof(InitData) );
	InitData.pSysMem = verts;
	wiRenderer::graphicsDevice->CreateBuffer( &bd, &InitData, &vertexBuffer );

	if(verts){
		delete[](verts);
		verts=NULL;
	}
}

void Lines::Transform(const XMFLOAT4X4& mat)
{
	desc.transform=mat;
}