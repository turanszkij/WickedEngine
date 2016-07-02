#include "wiTranslator.h"
#include "wiRenderer.h"
#include "wiInputManager.h"
#include "wiMath.h"

using namespace wiGraphicsTypes;

GPUBuffer* wiTranslator::vertexBuffer_Axis = nullptr;
GPUBuffer* wiTranslator::vertexBuffer_Plane = nullptr;
GPUBuffer* wiTranslator::vertexBuffer_Origin = nullptr;
int wiTranslator::vertexCount_Axis = 0;
int wiTranslator::vertexCount_Plane = 0;
int wiTranslator::vertexCount_Origin = 0;


wiTranslator::wiTranslator() :Transform()
{
	prevPointer = XMFLOAT4(0, 0, 0, 0);
	dragStart = XMFLOAT3(0, 0, 0);

	dragging = false;

	enabled = true;

	state = TRANSLATOR_IDLE;

	dist = 1;

	isTranslator = true; isScalator = false; isRotator = false;

	if (vertexBuffer_Axis == nullptr)
	{
		{
			XMFLOAT4 verts[] = {
				XMFLOAT4(0,0,0,1), XMFLOAT4(1,1,1,1),
				XMFLOAT4(3,0,0,1), XMFLOAT4(1,1,1,1),
			};
			vertexCount_Axis = ARRAYSIZE(verts) / 2;

			GPUBufferDesc bd;
			ZeroMemory(&bd, sizeof(bd));
			bd.Usage = USAGE_DEFAULT;
			bd.ByteWidth = sizeof(verts);
			bd.BindFlags = BIND_VERTEX_BUFFER;
			bd.CPUAccessFlags = 0;
			SubresourceData InitData;
			ZeroMemory(&InitData, sizeof(InitData));
			InitData.pSysMem = verts;
			vertexBuffer_Axis = new GPUBuffer;
			wiRenderer::GetDevice()->CreateBuffer(&bd, &InitData, vertexBuffer_Axis);
		}
	}

	if (vertexBuffer_Plane == nullptr)
	{
		{
			XMFLOAT4 verts[] = {
				XMFLOAT4(0,0,0,1), XMFLOAT4(1,1,1,1),
				XMFLOAT4(1,0,0,1), XMFLOAT4(1,1,1,1),
				XMFLOAT4(1,1,0,1), XMFLOAT4(1,1,1,1),

				XMFLOAT4(0,0,0,1), XMFLOAT4(1,1,1,1),
				XMFLOAT4(1,1,0,1), XMFLOAT4(1,1,1,1),
				XMFLOAT4(0,1,0,1), XMFLOAT4(1,1,1,1),
			};
			vertexCount_Plane = ARRAYSIZE(verts) / 2;

			GPUBufferDesc bd;
			ZeroMemory(&bd, sizeof(bd));
			bd.Usage = USAGE_DEFAULT;
			bd.ByteWidth = sizeof(verts);
			bd.BindFlags = BIND_VERTEX_BUFFER;
			bd.CPUAccessFlags = 0;
			SubresourceData InitData;
			ZeroMemory(&InitData, sizeof(InitData));
			InitData.pSysMem = verts;
			vertexBuffer_Plane = new GPUBuffer;
			wiRenderer::GetDevice()->CreateBuffer(&bd, &InitData, vertexBuffer_Plane);
		}
	}

	if (vertexBuffer_Origin == nullptr)
	{
		{
			float edge = 0.2f;
			XMFLOAT4 verts[] = {
				XMFLOAT4(-edge,edge,edge,1),   XMFLOAT4(1,1,1,1),
				XMFLOAT4(-edge,-edge,edge,1),  XMFLOAT4(1,1,1,1),
				XMFLOAT4(-edge,-edge,-edge,1), XMFLOAT4(1,1,1,1),
				XMFLOAT4(edge,edge,edge,1),	   XMFLOAT4(1,1,1,1),
				XMFLOAT4(edge,-edge,edge,1),   XMFLOAT4(1,1,1,1),
				XMFLOAT4(-edge,-edge,edge,1),  XMFLOAT4(1,1,1,1),
				XMFLOAT4(edge,edge,-edge,1),   XMFLOAT4(1,1,1,1),
				XMFLOAT4(edge,-edge,-edge,1),  XMFLOAT4(1,1,1,1),
				XMFLOAT4(edge,-edge,edge,1),   XMFLOAT4(1,1,1,1),
				XMFLOAT4(-edge,edge,-edge,1),  XMFLOAT4(1,1,1,1),
				XMFLOAT4(-edge,-edge,-edge,1), XMFLOAT4(1,1,1,1),
				XMFLOAT4(edge,-edge,-edge,1),  XMFLOAT4(1,1,1,1),
				XMFLOAT4(-edge,-edge,edge,1),  XMFLOAT4(1,1,1,1),
				XMFLOAT4(edge,-edge,edge,1),   XMFLOAT4(1,1,1,1),
				XMFLOAT4(edge,-edge,-edge,1),  XMFLOAT4(1,1,1,1),
				XMFLOAT4(edge,edge,edge,1),	   XMFLOAT4(1,1,1,1),
				XMFLOAT4(-edge,edge,edge,1),   XMFLOAT4(1,1,1,1),
				XMFLOAT4(-edge,edge,-edge,1),  XMFLOAT4(1,1,1,1),
				XMFLOAT4(-edge,edge,-edge,1),  XMFLOAT4(1,1,1,1),
				XMFLOAT4(-edge,edge,edge,1),   XMFLOAT4(1,1,1,1),
				XMFLOAT4(-edge,-edge,-edge,1), XMFLOAT4(1,1,1,1),
				XMFLOAT4(-edge,edge,edge,1),   XMFLOAT4(1,1,1,1),
				XMFLOAT4(edge,edge,edge,1),	   XMFLOAT4(1,1,1,1),
				XMFLOAT4(-edge,-edge,edge,1),  XMFLOAT4(1,1,1,1),
				XMFLOAT4(edge,edge,edge,1),	   XMFLOAT4(1,1,1,1),
				XMFLOAT4(edge,edge,-edge,1),   XMFLOAT4(1,1,1,1),
				XMFLOAT4(edge,-edge,edge,1),   XMFLOAT4(1,1,1,1),
				XMFLOAT4(edge,edge,-edge,1),   XMFLOAT4(1,1,1,1),
				XMFLOAT4(-edge,edge,-edge,1),  XMFLOAT4(1,1,1,1),
				XMFLOAT4(edge,-edge,-edge,1),  XMFLOAT4(1,1,1,1),
				XMFLOAT4(-edge,-edge,-edge,1), XMFLOAT4(1,1,1,1),
				XMFLOAT4(-edge,-edge,edge,1),  XMFLOAT4(1,1,1,1),
				XMFLOAT4(edge,-edge,-edge,1),  XMFLOAT4(1,1,1,1),
				XMFLOAT4(edge,edge,-edge,1),   XMFLOAT4(1,1,1,1),
				XMFLOAT4(edge,edge,edge,1),	   XMFLOAT4(1,1,1,1),
				XMFLOAT4(-edge,edge,-edge,1),  XMFLOAT4(1,1,1,1),
			};
			vertexCount_Origin = ARRAYSIZE(verts) / 2;

			GPUBufferDesc bd;
			ZeroMemory(&bd, sizeof(bd));
			bd.Usage = USAGE_DEFAULT;
			bd.ByteWidth = sizeof(verts);
			bd.BindFlags = BIND_VERTEX_BUFFER;
			bd.CPUAccessFlags = 0;
			SubresourceData InitData;
			ZeroMemory(&InitData, sizeof(InitData));
			InitData.pSysMem = verts;
			vertexBuffer_Origin = new GPUBuffer;
			wiRenderer::GetDevice()->CreateBuffer(&bd, &InitData, vertexBuffer_Origin);
		}
	}
}


wiTranslator::~wiTranslator()
{
}


void wiTranslator::Update()
{
	XMFLOAT4 pointer = wiInputManager::GetInstance()->getpointer();
	Camera* cam = wiRenderer::getCamera();
	XMVECTOR pos = XMLoadFloat3(&translation);

	if (enabled)
	{

		if (!dragging)
		{
			XMMATRIX P = cam->GetProjection();
			XMMATRIX V = cam->GetView();
			XMMATRIX W = XMMatrixIdentity();

			dist = wiMath::Distance(translation, cam->translation) * 0.05f;

			XMVECTOR o, x, y, z, p, xy, xz, yz;

			o = pos;
			x = o + XMVectorSet(3, 0, 0, 0) * dist;
			y = o + XMVectorSet(0, 3, 0, 0) * dist;
			z = o + XMVectorSet(0, 0, 3, 0) * dist;
			p = XMLoadFloat4(&pointer);
			xy = o + XMVectorSet(0.5f, 0.5f, 0, 0) * dist;
			xz = o + XMVectorSet(0.5f, 0, 0.5f, 0) * dist;
			yz = o + XMVectorSet(0, 0.5f, 0.5f, 0) * dist;


			o = XMVector3Project(o, 0, 0, cam->width, cam->height, 0, 1, P, V, W);
			x = XMVector3Project(x, 0, 0, cam->width, cam->height, 0, 1, P, V, W);
			y = XMVector3Project(y, 0, 0, cam->width, cam->height, 0, 1, P, V, W);
			z = XMVector3Project(z, 0, 0, cam->width, cam->height, 0, 1, P, V, W);
			xy = XMVector3Project(xy, 0, 0, cam->width, cam->height, 0, 1, P, V, W);
			xz = XMVector3Project(xz, 0, 0, cam->width, cam->height, 0, 1, P, V, W);
			yz = XMVector3Project(yz, 0, 0, cam->width, cam->height, 0, 1, P, V, W);

			XMVECTOR oDisV = XMVector3Length(o - p);
			XMVECTOR xyDisV = XMVector3Length(xy - p);
			XMVECTOR xzDisV = XMVector3Length(xz - p);
			XMVECTOR yzDisV = XMVector3Length(yz - p);

			float xDis = wiMath::GetPointSegmentDistance(p, o, x);
			float yDis = wiMath::GetPointSegmentDistance(p, o, y);
			float zDis = wiMath::GetPointSegmentDistance(p, o, z);
			float oDis = XMVectorGetX(oDisV);
			float xyDis = XMVectorGetX(xyDisV);
			float xzDis = XMVectorGetX(xzDisV);
			float yzDis = XMVectorGetX(yzDisV);

			if (oDis < 10)
			{
				state = TRANSLATOR_XYZ;
			}
			else if (xyDis < 20)
			{
				state = TRANSLATOR_XY;
			}
			else if (xzDis < 20)
			{
				state = TRANSLATOR_XZ;
			}
			else if (yzDis < 20)
			{
				state = TRANSLATOR_YZ;
			}
			else if (xDis < 10)
			{
				state = TRANSLATOR_X;
			}
			else if (yDis < 10)
			{
				state = TRANSLATOR_Y;
			}
			else if (zDis < 10)
			{
				state = TRANSLATOR_Z;
			}
			else if (!dragging)
			{
				state = TRANSLATOR_IDLE;
			}
		}

		if (dragging || (state != TRANSLATOR_IDLE && wiInputManager::GetInstance()->down(VK_LBUTTON)))
		{
			XMVECTOR plane, planeNormal;
			if (state == TRANSLATOR_X)
			{
				XMVECTOR axis = XMVectorSet(1, 0, 0, 0);
				XMVECTOR wrong = XMVector3Cross(cam->GetAt(), axis);
				planeNormal = XMVector3Cross(wrong, axis);
			}
			else if (state == TRANSLATOR_Y)
			{
				XMVECTOR axis = XMVectorSet(0, 1, 0, 0);
				XMVECTOR wrong = XMVector3Cross(cam->GetAt(), axis);
				planeNormal = XMVector3Cross(wrong, axis);
			}
			else if (state == TRANSLATOR_Z)
			{
				XMVECTOR axis = XMVectorSet(0, 0, 1, 0);
				XMVECTOR wrong = XMVector3Cross(cam->GetAt(), axis);
				planeNormal = XMVector3Cross(wrong, axis);
			}
			else if (state == TRANSLATOR_XY)
			{
				planeNormal = XMVectorSet(0, 0, 1, 0);
			}
			else if (state == TRANSLATOR_XZ)
			{
				planeNormal = XMVectorSet(0, 1, 0, 0);
			}
			else if (state == TRANSLATOR_YZ)
			{
				planeNormal = XMVectorSet(1, 0, 0, 0);
			}
			else
			{
				// xyz
				planeNormal = cam->GetAt();
			}
			plane = XMPlaneFromPointNormal(pos, XMVector3Normalize(planeNormal));

			RAY ray = wiRenderer::getPickRay((long)pointer.x, (long)pointer.y);
			XMVECTOR rayOrigin = XMLoadFloat3(&ray.origin);
			XMVECTOR rayDir = XMLoadFloat3(&ray.direction);
			XMVECTOR intersection = XMPlaneIntersectLine(plane, rayOrigin, rayOrigin + rayDir*cam->zFarP);

			ray = wiRenderer::getPickRay((long)prevPointer.x, (long)prevPointer.y);
			rayOrigin = XMLoadFloat3(&ray.origin);
			rayDir = XMLoadFloat3(&ray.direction);
			XMVECTOR intersectionPrev = XMPlaneIntersectLine(plane, rayOrigin, rayOrigin + rayDir*cam->zFarP);


			XMFLOAT3 delta;
			if (state == TRANSLATOR_X)
			{
				XMVECTOR A = pos, B = pos + XMVectorSet(1, 0, 0, 0);
				XMVECTOR P = wiMath::GetClosestPointToLine(A, B, intersection);
				XMVECTOR PPrev = wiMath::GetClosestPointToLine(A, B, intersectionPrev);
				XMStoreFloat3(&delta, P - PPrev);
			}
			else if (state == TRANSLATOR_Y)
			{
				XMVECTOR A = pos, B = pos + XMVectorSet(0, 1, 0, 0);
				XMVECTOR P = wiMath::GetClosestPointToLine(A, B, intersection);
				XMVECTOR PPrev = wiMath::GetClosestPointToLine(A, B, intersectionPrev);
				XMStoreFloat3(&delta, P - PPrev);
			}
			else if (state == TRANSLATOR_Z)
			{
				XMVECTOR A = pos, B = pos + XMVectorSet(0, 0, 1, 0);
				XMVECTOR P = wiMath::GetClosestPointToLine(A, B, intersection);
				XMVECTOR PPrev = wiMath::GetClosestPointToLine(A, B, intersectionPrev);
				XMStoreFloat3(&delta, P - PPrev);
			}
			else
			{
				XMVECTOR deltaV = intersection - intersectionPrev;
				if (isScalator)
				{
					float sca = XMVectorGetY(deltaV);
					delta = XMFLOAT3(sca, sca, sca);
				}
				else
				{
					XMStoreFloat3(&delta, deltaV);
				}
			}


			XMMATRIX transf = XMMatrixIdentity();

			if (isTranslator)
			{
				transf *= XMMatrixTranslation(delta.x, delta.y, delta.z);
			}
			if (isRotator)
			{
				transf *= XMMatrixRotationRollPitchYaw(delta.x, delta.y, delta.z);
			}
			if (isScalator)
			{
				transf *= XMMatrixScaling((1.0f / scale.x) * (scale.x + delta.x), (1.0f / scale.y) * (scale.y + delta.y), (1.0f / scale.z) * (scale.z + delta.z));
			}

			Transform::transform(transf);

			Transform::applyTransform();

			dragging = true;
		}

		if (!wiInputManager::GetInstance()->down(VK_LBUTTON))
		{
			dragging = false;
			Transform::UpdateTransform();
		}

	}
	else
	{
		dragging = false;
		Transform::UpdateTransform();
	}

	prevPointer = pointer;
}
