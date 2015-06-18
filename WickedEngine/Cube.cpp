#include "Cube.h"


ID3D11Buffer* Cube::vertexBuffer;
ID3D11Buffer* Cube::indexBuffer;

Cube::Cube(const XMFLOAT3& center, const XMFLOAT3& halfwidth, const XMFLOAT4A& color)
{
	desc=Description();
	desc.center=center;
	desc.halfwidth=halfwidth;
	desc.color=color;
	Transform(XMMatrixScaling(halfwidth.x,halfwidth.y,halfwidth.z)*XMMatrixTranslation(center.x,center.y,center.z));
}


void Cube::CleanUpStatic()
{
	if(vertexBuffer){
		vertexBuffer->Release();
		vertexBuffer = NULL;
	}
	if(indexBuffer){
		indexBuffer->Release();
		indexBuffer = NULL;
	}
}

void Cube::SetUpVertices()
{
	vertexBuffer=NULL;
	indexBuffer=NULL;

	Vertex* verts = new Vertex[8];

	XMFLOAT3A min = XMFLOAT3A(-1,-1,-1);
	XMFLOAT3A max = XMFLOAT3A(1,1,1);
	verts[0].pos=min;
	verts[1].pos=XMFLOAT3A(min.x,max.y,min.z);
	verts[2].pos=XMFLOAT3A(min.x,max.y,max.z);
	verts[3].pos=XMFLOAT3A(min.x,min.y,max.z);
	verts[4].pos=XMFLOAT3A(max.x,min.y,min.z);
	verts[5].pos=XMFLOAT3A(max.x,max.y,min.z);
	verts[6].pos=max;
	verts[7].pos=XMFLOAT3A(max.x,min.y,max.z);

	D3D11_BUFFER_DESC bd;
	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof( Vertex ) * 8;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	D3D11_SUBRESOURCE_DATA InitData;
	ZeroMemory( &InitData, sizeof(InitData) );
	InitData.pSysMem = verts;
	Renderer::graphicsDevice->CreateBuffer( &bd, &InitData, &vertexBuffer );

	if(verts){
		delete[](verts);
		verts=NULL;
	}

	unsigned int indices[]={
		0,1,1,2,0,3,0,4,1,5,4,5,
		5,6,4,7,2,6,3,7,2,3,6,7
	};

	ZeroMemory( &bd, sizeof(bd) );
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof( unsigned int ) * 24;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;
	ZeroMemory( &InitData, sizeof(InitData) );
	InitData.pSysMem = indices;
	Renderer::graphicsDevice->CreateBuffer( &bd, &InitData, &indexBuffer );
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