#include "wiCube.h"
#include "wiRenderer.h"

using namespace wiGraphicsTypes;

GPUBuffer Cube::vertexBuffer;
GPUBuffer Cube::indexBuffer;

Cube::Cube(const XMFLOAT3& center, const XMFLOAT3& halfwidth, const XMFLOAT4& color)
{
	desc=Description();
	desc.center=center;
	desc.halfwidth=halfwidth;
	desc.color=color;
	Transform(XMMatrixScaling(halfwidth.x,halfwidth.y,halfwidth.z)*XMMatrixTranslation(center.x,center.y,center.z));
}


void Cube::CleanUpStatic()
{
	
}

void Cube::SetUpVertices()
{

	//Vertex* verts = new Vertex[8];

	XMFLOAT4 min = XMFLOAT4(-1, -1, -1, 1);
	XMFLOAT4 max = XMFLOAT4(1, 1, 1, 1);
	//verts[0].pos=min;
	//verts[1].pos=XMFLOAT3(min.x,max.y,min.z);
	//verts[2].pos=XMFLOAT3(min.x,max.y,max.z);
	//verts[3].pos=XMFLOAT3(min.x,min.y,max.z);
	//verts[4].pos=XMFLOAT3(max.x,min.y,min.z);
	//verts[5].pos=XMFLOAT3(max.x,max.y,min.z);
	//verts[6].pos=max;
	//verts[7].pos=XMFLOAT3(max.x,min.y,max.z);

	XMFLOAT4 verts[] = {
		min,							XMFLOAT4(1,1,1,1),
		XMFLOAT4(min.x,max.y,min.z,1),	XMFLOAT4(1,1,1,1),
		XMFLOAT4(min.x,max.y,max.z,1),	XMFLOAT4(1,1,1,1),
		XMFLOAT4(min.x,min.y,max.z,1),	XMFLOAT4(1,1,1,1),
		XMFLOAT4(max.x,min.y,min.z,1),	XMFLOAT4(1,1,1,1),
		XMFLOAT4(max.x,max.y,min.z,1),	XMFLOAT4(1,1,1,1),
		max,							XMFLOAT4(1,1,1,1),
		XMFLOAT4(max.x,min.y,max.z,1),	XMFLOAT4(1,1,1,1),
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

	unsigned int indices[]={
		0,1,1,2,0,3,0,4,1,5,4,5,
		5,6,4,7,2,6,3,7,2,3,6,7
	};

	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = USAGE_DEFAULT;
	bd.ByteWidth = sizeof(indices);
	bd.BindFlags = BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;
	ZeroMemory( &InitData, sizeof(InitData) );
	InitData.pSysMem = indices;
	wiRenderer::GetDevice()->CreateBuffer( &bd, &InitData, &indexBuffer );
}

void Cube::Transform(const XMFLOAT4X4& mat)
{
	desc.transform=mat;
}
void Cube::Transform(const XMMATRIX& mat)
{
	XMStoreFloat4x4( &desc.transform,mat );
}

void Cube::LoadStatic(){
	SetUpVertices();
}