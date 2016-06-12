#include "wiTranslator.h"
#include "wiRenderer.h"
#include "wiInputManager.h"
#include "wiMath.h"

using namespace wiGraphicsTypes;

GPUBuffer* wiTranslator::vertexBuffer = nullptr;
int wiTranslator::vertexCount = 0;


wiTranslator::wiTranslator() :Transform()
{
	prevPointer = XMFLOAT4(0, 0, 0, 0);

	enabled = true;

	if (vertexBuffer == nullptr)
	{
		vertexBuffer = new GPUBuffer;

		XMFLOAT4 verts[] = {
			XMFLOAT4(0,0,0,1), XMFLOAT4(1,0,0,1),
			XMFLOAT4(1,0,0,1), XMFLOAT4(1,0,0,1),

			XMFLOAT4(0,0,0,1), XMFLOAT4(0,1,0,1),
			XMFLOAT4(0,1,0,1), XMFLOAT4(0,1,0,1),

			XMFLOAT4(0,0,0,1), XMFLOAT4(0,0,1,1),
			XMFLOAT4(0,0,1,1), XMFLOAT4(0,0,1,1),
		};
		vertexCount = ARRAYSIZE(verts);

		GPUBufferDesc bd;
		ZeroMemory(&bd, sizeof(bd));
		bd.Usage = USAGE_DEFAULT;
		bd.ByteWidth = sizeof(verts);
		bd.BindFlags = BIND_VERTEX_BUFFER;
		bd.CPUAccessFlags = 0;
		SubresourceData InitData;
		ZeroMemory(&InitData, sizeof(InitData));
		InitData.pSysMem = verts;
		wiRenderer::GetDevice()->CreateBuffer(&bd, &InitData, vertexBuffer);
	}
}


wiTranslator::~wiTranslator()
{
}

void wiTranslator::Update()
{
	XMFLOAT4 pointer = wiInputManager::GetInstance()->getpointer();
	if (enabled && wiInputManager::GetInstance()->down(VK_LBUTTON))
	{
		Camera* cam = wiRenderer::getCamera();
		float dist = wiMath::Distance(cam->translation, translation);


		RAY prevRay = wiRenderer::getPickRay((long)prevPointer.x, (long)prevPointer.y);
		RAY ray = wiRenderer::getPickRay((long)pointer.x, (long)pointer.y);

		XMFLOAT3 prevGrab = wiMath::Lerp(prevRay.origin, prevRay.direction, dist);
		XMFLOAT3 grab = wiMath::Lerp(ray.origin, ray.direction, dist);

		XMFLOAT3 delta;
		XMStoreFloat3(&delta, XMVectorSubtract(XMLoadFloat3(&grab), XMLoadFloat3(&prevGrab)));

		Translate(delta);

		Transform::UpdateTransform();
	}
	prevPointer = pointer;
}
