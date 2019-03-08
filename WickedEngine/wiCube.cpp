#include "wiCube.h"
#include "wiRenderer.h"

using namespace wiGraphics;

static GPUBuffer vertexBuffer;
static GPUBuffer indexBuffer;

wiCube::wiCube(const XMFLOAT3& center, const XMFLOAT3& halfwidth, const XMFLOAT4& color)
{
	desc=Description();
	desc.center=center;
	desc.halfwidth=halfwidth;
	desc.color=color;
	Transform(XMMatrixScaling(halfwidth.x,halfwidth.y,halfwidth.z)*XMMatrixTranslation(center.x,center.y,center.z));
}


void wiCube::Transform(const XMFLOAT4X4& mat)
{
	desc.transform=mat;
}
void wiCube::Transform(const XMMATRIX& mat)
{
	XMStoreFloat4x4( &desc.transform,mat );
}


GPUBuffer* wiCube::GetVertexBuffer()
{
	return &vertexBuffer;
}
GPUBuffer* wiCube::GetIndexBuffer()
{
	return &indexBuffer;
}

void wiCube::Initialize()
{
	XMFLOAT4 min = XMFLOAT4(-1, -1, -1, 1);
	XMFLOAT4 max = XMFLOAT4(1, 1, 1, 1);

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
	bd.Usage = USAGE_DEFAULT;
	bd.ByteWidth = sizeof(verts);
	bd.BindFlags = BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	SubresourceData InitData;
	InitData.pSysMem = verts;
	wiRenderer::GetDevice()->CreateBuffer(&bd, &InitData, &vertexBuffer);

	uint16_t indices[] = {
		0,1,1,2,0,3,0,4,1,5,4,5,
		5,6,4,7,2,6,3,7,2,3,6,7
	};

	bd.Usage = USAGE_DEFAULT;
	bd.ByteWidth = sizeof(indices);
	bd.BindFlags = BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;
	InitData.pSysMem = indices;
	wiRenderer::GetDevice()->CreateBuffer(&bd, &InitData, &indexBuffer);


}
void wiCube::CleanUp()
{
	
}
