#include "stdafx.h"
#include "Translator.h"
#include "wiRenderer.h"
#include "wiInput.h"
#include "wiMath.h"
#include "ShaderInterop_Renderer.h"
#include "wiEvent.h"

using namespace wiGraphics;
using namespace wiECS;
using namespace wiScene;

PipelineState pso_solidpart;
PipelineState pso_wirepart;
GPUBuffer vertexBuffer_Axis;
GPUBuffer vertexBuffer_Plane;
GPUBuffer vertexBuffer_Origin;
uint32_t vertexCount_Axis = 0;
uint32_t vertexCount_Plane = 0;
uint32_t vertexCount_Origin = 0;
float origin_size = 0.2f;

namespace Translator_Internal
{
	void LoadShaders()
	{
		GraphicsDevice* device = wiRenderer::GetDevice();

		{
			PipelineStateDesc desc;

			desc.vs = wiRenderer::GetVertexShader(VSTYPE_VERTEXCOLOR);
			desc.ps = wiRenderer::GetPixelShader(PSTYPE_VERTEXCOLOR);
			desc.il = wiRenderer::GetInputLayout(ILTYPE_VERTEXCOLOR);
			desc.dss = wiRenderer::GetDepthStencilState(DSSTYPE_XRAY);
			desc.rs = wiRenderer::GetRasterizerState(RSTYPE_DOUBLESIDED);
			desc.bs = wiRenderer::GetBlendState(BSTYPE_ADDITIVE);
			desc.pt = TRIANGLELIST;

			device->CreatePipelineState(&desc, &pso_solidpart);
		}

		{
			PipelineStateDesc desc;

			desc.vs = wiRenderer::GetVertexShader(VSTYPE_VERTEXCOLOR);
			desc.ps = wiRenderer::GetPixelShader(PSTYPE_VERTEXCOLOR);
			desc.il = wiRenderer::GetInputLayout(ILTYPE_VERTEXCOLOR);
			desc.dss = wiRenderer::GetDepthStencilState(DSSTYPE_XRAY);
			desc.rs = wiRenderer::GetRasterizerState(RSTYPE_WIRE_DOUBLESIDED_SMOOTH);
			desc.bs = wiRenderer::GetBlendState(BSTYPE_TRANSPARENT);
			desc.pt = LINELIST;

			device->CreatePipelineState(&desc, &pso_wirepart);
		}
	}
}

void Translator::Create()
{
	GraphicsDevice* device = wiRenderer::GetDevice();

	if (!vertexBuffer_Axis.IsValid())
	{
		{
			XMFLOAT4 verts[] = {
				XMFLOAT4(0,0,0,1), XMFLOAT4(1,1,1,1),
				XMFLOAT4(3,0,0,1), XMFLOAT4(1,1,1,1),
			};
			vertexCount_Axis = arraysize(verts) / 2;

			GPUBufferDesc bd;
			bd.Usage = USAGE_DEFAULT;
			bd.ByteWidth = sizeof(verts);
			bd.BindFlags = BIND_VERTEX_BUFFER;
			bd.CPUAccessFlags = 0;

			SubresourceData InitData;
			InitData.pSysMem = verts;

			device->CreateBuffer(&bd, &InitData, &vertexBuffer_Axis);
		}
	}

	if (!vertexBuffer_Plane.IsValid())
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
			vertexCount_Plane = arraysize(verts) / 2;

			GPUBufferDesc bd;
			bd.Usage = USAGE_DEFAULT;
			bd.ByteWidth = sizeof(verts);
			bd.BindFlags = BIND_VERTEX_BUFFER;
			bd.CPUAccessFlags = 0;

			SubresourceData InitData;
			InitData.pSysMem = verts;
			device->CreateBuffer(&bd, &InitData, &vertexBuffer_Plane);
		}
	}

	if (!vertexBuffer_Origin.IsValid())
	{
		{
			float edge = origin_size;
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
			vertexCount_Origin = arraysize(verts) / 2;

			GPUBufferDesc bd;
			bd.Usage = USAGE_DEFAULT;
			bd.ByteWidth = sizeof(verts);
			bd.BindFlags = BIND_VERTEX_BUFFER;
			bd.CPUAccessFlags = 0;

			SubresourceData InitData;
			InitData.pSysMem = verts;
			device->CreateBuffer(&bd, &InitData, &vertexBuffer_Origin);
		}
	}
}

void Translator::Update()
{
	if (selected.empty())
	{
		return;
	}

	dragStarted = false;
	dragEnded = false;

	XMFLOAT4 pointer = wiInput::GetPointer();
	const CameraComponent& cam = wiRenderer::GetCamera();
	XMVECTOR pos = transform.GetPositionV();

	if (enabled)
	{
		PreTranslate();

		if (!dragging)
		{
			XMMATRIX P = cam.GetProjection();
			XMMATRIX V = cam.GetView();
			XMMATRIX W = XMMatrixIdentity();
			XMFLOAT3 p = transform.GetPosition();

			dist = wiMath::Distance(p, cam.Eye) * 0.05f;

			RAY ray = wiRenderer::GetPickRay((long)pointer.x, (long)pointer.y);

			XMVECTOR x, y, z, xy, xz, yz;

			x = pos + XMVectorSet(3, 0, 0, 0) * dist;
			y = pos + XMVectorSet(0, 3, 0, 0) * dist;
			z = pos + XMVectorSet(0, 0, 3, 0) * dist;
			xy = pos + XMVectorSet(1, 1, 0, 0) * dist;
			xz = pos + XMVectorSet(1, 0, 1, 0) * dist;
			yz = pos + XMVectorSet(0, 1, 1, 0) * dist;

			AABB aabb_origin;
			aabb_origin.createFromHalfWidth(p, XMFLOAT3(origin_size * dist, origin_size * dist, origin_size * dist));

			XMFLOAT3 maxp;
			XMStoreFloat3(&maxp, x);
			AABB aabb_x = AABB::Merge(AABB(p, maxp), aabb_origin);

			XMStoreFloat3(&maxp, y);
			AABB aabb_y = AABB::Merge(AABB(p, maxp), aabb_origin);

			XMStoreFloat3(&maxp, z);
			AABB aabb_z = AABB::Merge(AABB(p, maxp), aabb_origin);

			XMStoreFloat3(&maxp, xy);
			AABB aabb_xy = AABB(p, maxp);

			XMStoreFloat3(&maxp, xz);
			AABB aabb_xz = AABB(p, maxp);

			XMStoreFloat3(&maxp, yz);
			AABB aabb_yz = AABB(p, maxp);

			if (aabb_origin.intersects(ray))
			{
				state = TRANSLATOR_XYZ;
			}
			else if (aabb_x.intersects(ray))
			{
				state = TRANSLATOR_X;
			}
			else if (aabb_y.intersects(ray))
			{
				state = TRANSLATOR_Y;
			}
			else if (aabb_z.intersects(ray))
			{
				state = TRANSLATOR_Z;
			}
			else if (!dragging)
			{
				state = TRANSLATOR_IDLE;
			}

			if (state != TRANSLATOR_XYZ)
			{
				// these can overlap, so take closest one (by checking plane ray trace distance):
				XMVECTOR origin = XMLoadFloat3(&ray.origin);
				XMVECTOR direction = XMLoadFloat3(&ray.direction);
				XMVECTOR N = XMVectorSet(0, 0, 1, 0);

				float prio = FLT_MAX;
				if (aabb_xy.intersects(ray))
				{
					state = TRANSLATOR_XY;
					prio = XMVectorGetX(XMVector3Dot(N, (origin - pos) / XMVectorAbs(XMVector3Dot(N, direction))));
				}

				N = XMVectorSet(0, 1, 0, 0);
				float d = XMVectorGetX(XMVector3Dot(N, (origin - pos) / XMVectorAbs(XMVector3Dot(N, direction))));
				if (d < prio && aabb_xz.intersects(ray))
				{
					state = TRANSLATOR_XZ;
					prio = d;
				}

				N = XMVectorSet(1, 0, 0, 0);
				d = XMVectorGetX(XMVector3Dot(N, (origin - pos) / XMVectorAbs(XMVector3Dot(N, direction))));
				if (d < prio && aabb_yz.intersects(ray))
				{
					state = TRANSLATOR_YZ;
				}
			}
		}

		if (dragging || (state != TRANSLATOR_IDLE && wiInput::Press(wiInput::MOUSE_BUTTON_LEFT)))
		{
			if (!dragging)
			{
				dragStarted = true;
				dragDeltaMatrix = IDENTITYMATRIX;
			}

			XMVECTOR plane, planeNormal;
			if (state == TRANSLATOR_X)
			{
				XMVECTOR axis = XMVectorSet(1, 0, 0, 0);
				XMVECTOR wrong = XMVector3Cross(cam.GetAt(), axis);
				planeNormal = XMVector3Cross(wrong, axis);
			}
			else if (state == TRANSLATOR_Y)
			{
				XMVECTOR axis = XMVectorSet(0, 1, 0, 0);
				XMVECTOR wrong = XMVector3Cross(cam.GetAt(), axis);
				planeNormal = XMVector3Cross(wrong, axis);
			}
			else if (state == TRANSLATOR_Z)
			{
				XMVECTOR axis = XMVectorSet(0, 0, 1, 0);
				XMVECTOR wrong = XMVector3Cross(cam.GetAt(), axis);
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
				planeNormal = cam.GetAt();
			}
			plane = XMPlaneFromPointNormal(pos, XMVector3Normalize(planeNormal));

			RAY ray = wiRenderer::GetPickRay((long)pointer.x, (long)pointer.y);
			XMVECTOR rayOrigin = XMLoadFloat3(&ray.origin);
			XMVECTOR rayDir = XMLoadFloat3(&ray.direction);
			XMVECTOR intersection = XMPlaneIntersectLine(plane, rayOrigin, rayOrigin + rayDir*cam.zFarP);

			ray = wiRenderer::GetPickRay((long)prevPointer.x, (long)prevPointer.y);
			rayOrigin = XMLoadFloat3(&ray.origin);
			rayDir = XMLoadFloat3(&ray.direction);
			XMVECTOR intersectionPrev = XMPlaneIntersectLine(plane, rayOrigin, rayOrigin + rayDir*cam.zFarP);

			XMVECTOR deltaV;
			if (state == TRANSLATOR_X)
			{
				XMVECTOR A = pos, B = pos + XMVectorSet(1, 0, 0, 0);
				XMVECTOR P = wiMath::GetClosestPointToLine(A, B, intersection);
				XMVECTOR PPrev = wiMath::GetClosestPointToLine(A, B, intersectionPrev);
				deltaV = P - PPrev;
			}
			else if (state == TRANSLATOR_Y)
			{
				XMVECTOR A = pos, B = pos + XMVectorSet(0, 1, 0, 0);
				XMVECTOR P = wiMath::GetClosestPointToLine(A, B, intersection);
				XMVECTOR PPrev = wiMath::GetClosestPointToLine(A, B, intersectionPrev);
				deltaV = P - PPrev;
			}
			else if (state == TRANSLATOR_Z)
			{
				XMVECTOR A = pos, B = pos + XMVectorSet(0, 0, 1, 0);
				XMVECTOR P = wiMath::GetClosestPointToLine(A, B, intersection);
				XMVECTOR PPrev = wiMath::GetClosestPointToLine(A, B, intersectionPrev);
				deltaV = P - PPrev;
			}
			else
			{
				deltaV = intersection - intersectionPrev;

				if (isScalator)
				{
					deltaV = XMVectorSplatY(deltaV);
				}
			}

			XMFLOAT3 delta;
			if (isRotator)
			{
				deltaV /= XMVector3Length(intersection - rayOrigin);
				deltaV *= XM_2PI;
			}
			XMStoreFloat3(&delta, deltaV);

			if (isTranslator)
			{
				XMStoreFloat4x4(&dragDeltaMatrix, XMMatrixTranslation(delta.x, delta.y, delta.z) * XMLoadFloat4x4(&dragDeltaMatrix));
				transform.Translate(delta);
			}
			if (isRotator)
			{
				XMMATRIX R = XMMatrixRotationRollPitchYaw(delta.x, delta.y, delta.z);
				XMStoreFloat4x4(&dragDeltaMatrix, R * XMLoadFloat4x4(&dragDeltaMatrix));
				XMVECTOR Q = XMQuaternionRotationMatrix(R);
				XMFLOAT4 quat;
				XMStoreFloat4(&quat, Q);
				transform.Rotate(quat);
			}
			if (isScalator)
			{
				XMFLOAT3 scale = transform.GetScale();
				scale = XMFLOAT3((1.0f / scale.x) * (scale.x + delta.x), (1.0f / scale.y) * (scale.y + delta.y), (1.0f / scale.z) * (scale.z + delta.z));
				XMStoreFloat4x4(&dragDeltaMatrix, XMMatrixScaling(scale.x, scale.y, scale.z) * XMLoadFloat4x4(&dragDeltaMatrix));
				transform.Scale(scale);
			}
			transform.UpdateTransform();

			dragging = true;
		}

		if (!wiInput::Down(wiInput::MOUSE_BUTTON_LEFT))
		{
			if (dragging)
			{
				dragEnded = true;
			}
			dragging = false;
		}

		PostTranslate();

	}
	else
	{
		if (dragging)
		{
			dragEnded = true;
		}
		dragging = false;
	}

	prevPointer = pointer;
}
void Translator::Draw(const CameraComponent& camera, CommandList cmd) const
{
	if (selected.empty())
	{
		return;
	}

	static bool shaders_loaded = false;
	if (!shaders_loaded)
	{
		shaders_loaded = true;
		static wiEvent::Handle handle = wiEvent::Subscribe(SYSTEM_EVENT_RELOAD_SHADERS, [](uint64_t userdata) { Translator_Internal::LoadShaders(); });
		Translator_Internal::LoadShaders();
	}

	Scene& scene = wiScene::GetScene();

	GraphicsDevice* device = wiRenderer::GetDevice();

	device->EventBegin("Editor - Translator", cmd);

	XMMATRIX VP = camera.GetViewProjection();

	MiscCB sb;

	XMMATRIX mat = XMMatrixScaling(dist, dist, dist)*XMMatrixTranslationFromVector(transform.GetPositionV()) * VP;
	XMMATRIX matX = mat;
	XMMATRIX matY = XMMatrixRotationZ(XM_PIDIV2)*XMMatrixRotationY(XM_PIDIV2)*mat;
	XMMATRIX matZ = XMMatrixRotationY(-XM_PIDIV2)*XMMatrixRotationZ(-XM_PIDIV2)*mat;

	// Planes:
	{
		device->BindPipelineState(&pso_solidpart, cmd);
		const GPUBuffer* vbs[] = {
			&vertexBuffer_Plane,
		};
		const uint32_t strides[] = {
			sizeof(XMFLOAT4) + sizeof(XMFLOAT4),
		};
		device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, nullptr, cmd);
	}

	// xy
	XMStoreFloat4x4(&sb.g_xTransform, matX);
	sb.g_xColor = state == TRANSLATOR_XY ? XMFLOAT4(1, 1, 1, 1) : XMFLOAT4(0.4f, 0.4f, 0, 0.4f);
	device->UpdateBuffer(wiRenderer::GetConstantBuffer(CBTYPE_MISC), &sb, cmd);
	device->BindConstantBuffer(VS, wiRenderer::GetConstantBuffer(CBTYPE_MISC), CB_GETBINDSLOT(MiscCB), cmd);
	device->BindConstantBuffer(PS, wiRenderer::GetConstantBuffer(CBTYPE_MISC), CB_GETBINDSLOT(MiscCB), cmd);
	device->Draw(vertexCount_Plane, 0, cmd);

	// xz
	XMStoreFloat4x4(&sb.g_xTransform, matZ);
	sb.g_xColor = state == TRANSLATOR_XZ ? XMFLOAT4(1, 1, 1, 1) : XMFLOAT4(0.4f, 0.4f, 0, 0.4f);
	device->UpdateBuffer(wiRenderer::GetConstantBuffer(CBTYPE_MISC), &sb, cmd);
	device->BindConstantBuffer(VS, wiRenderer::GetConstantBuffer(CBTYPE_MISC), CB_GETBINDSLOT(MiscCB), cmd);
	device->BindConstantBuffer(PS, wiRenderer::GetConstantBuffer(CBTYPE_MISC), CB_GETBINDSLOT(MiscCB), cmd);
	device->Draw(vertexCount_Plane, 0, cmd);

	// yz
	XMStoreFloat4x4(&sb.g_xTransform, matY);
	sb.g_xColor = state == TRANSLATOR_YZ ? XMFLOAT4(1, 1, 1, 1) : XMFLOAT4(0.4f, 0.4f, 0, 0.4f);
	device->UpdateBuffer(wiRenderer::GetConstantBuffer(CBTYPE_MISC), &sb, cmd);
	device->BindConstantBuffer(VS, wiRenderer::GetConstantBuffer(CBTYPE_MISC), CB_GETBINDSLOT(MiscCB), cmd);
	device->BindConstantBuffer(PS, wiRenderer::GetConstantBuffer(CBTYPE_MISC), CB_GETBINDSLOT(MiscCB), cmd);
	device->Draw(vertexCount_Plane, 0, cmd);

	// Lines:
	{
		device->BindPipelineState(&pso_wirepart, cmd);
		const GPUBuffer* vbs[] = {
			&vertexBuffer_Axis,
		};
		const uint32_t strides[] = {
			sizeof(XMFLOAT4) + sizeof(XMFLOAT4),
		};
		device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, nullptr, cmd);
	}

	// x
	XMStoreFloat4x4(&sb.g_xTransform, matX);
	sb.g_xColor = state == TRANSLATOR_X ? XMFLOAT4(1, 1, 1, 1) : XMFLOAT4(1, 0, 0, 1);
	device->UpdateBuffer(wiRenderer::GetConstantBuffer(CBTYPE_MISC), &sb, cmd);
	device->BindConstantBuffer(VS, wiRenderer::GetConstantBuffer(CBTYPE_MISC), CB_GETBINDSLOT(MiscCB), cmd);
	device->BindConstantBuffer(PS, wiRenderer::GetConstantBuffer(CBTYPE_MISC), CB_GETBINDSLOT(MiscCB), cmd);
	device->Draw(vertexCount_Axis, 0, cmd);

	// y
	XMStoreFloat4x4(&sb.g_xTransform, matY);
	sb.g_xColor = state == TRANSLATOR_Y ? XMFLOAT4(1, 1, 1, 1) : XMFLOAT4(0, 1, 0, 1);
	device->UpdateBuffer(wiRenderer::GetConstantBuffer(CBTYPE_MISC), &sb, cmd);
	device->BindConstantBuffer(VS, wiRenderer::GetConstantBuffer(CBTYPE_MISC), CB_GETBINDSLOT(MiscCB), cmd);
	device->BindConstantBuffer(PS, wiRenderer::GetConstantBuffer(CBTYPE_MISC), CB_GETBINDSLOT(MiscCB), cmd);
	device->Draw(vertexCount_Axis, 0, cmd);

	// z
	XMStoreFloat4x4(&sb.g_xTransform, matZ);
	sb.g_xColor = state == TRANSLATOR_Z ? XMFLOAT4(1, 1, 1, 1) : XMFLOAT4(0, 0, 1, 1);
	device->UpdateBuffer(wiRenderer::GetConstantBuffer(CBTYPE_MISC), &sb, cmd);
	device->BindConstantBuffer(VS, wiRenderer::GetConstantBuffer(CBTYPE_MISC), CB_GETBINDSLOT(MiscCB), cmd);
	device->BindConstantBuffer(PS, wiRenderer::GetConstantBuffer(CBTYPE_MISC), CB_GETBINDSLOT(MiscCB), cmd);
	device->Draw(vertexCount_Axis, 0, cmd);

	// Origin:
	{
		device->BindPipelineState(&pso_solidpart, cmd);
		const GPUBuffer* vbs[] = {
			&vertexBuffer_Origin,
		};
		const uint32_t strides[] = {
			sizeof(XMFLOAT4) + sizeof(XMFLOAT4),
		};
		device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, nullptr, cmd);
		XMStoreFloat4x4(&sb.g_xTransform, mat);
		sb.g_xColor = state == TRANSLATOR_XYZ ? XMFLOAT4(1, 1, 1, 1) : XMFLOAT4(0.25f, 0.25f, 0.25f, 1);
		device->UpdateBuffer(wiRenderer::GetConstantBuffer(CBTYPE_MISC), &sb, cmd);
		device->BindConstantBuffer(VS, wiRenderer::GetConstantBuffer(CBTYPE_MISC), CB_GETBINDSLOT(MiscCB), cmd);
		device->BindConstantBuffer(PS, wiRenderer::GetConstantBuffer(CBTYPE_MISC), CB_GETBINDSLOT(MiscCB), cmd);
		device->Draw(vertexCount_Origin, 0, cmd);
	}

	device->EventEnd(cmd);
}

void Translator::PreTranslate()
{
	Scene& scene = wiScene::GetScene();

	// Find the center of all the entities that are selected:
	XMVECTOR centerV = XMVectorSet(0, 0, 0, 0);
	float count = 0;
	for (auto& x : selected)
	{
		TransformComponent* transform = scene.transforms.GetComponent(x.entity);
		if (transform != nullptr)
		{
			centerV = XMVectorAdd(centerV, transform->GetPositionV());
			count += 1.0f;
		}
	}

	// Offset translator to center position and perform attachments:
	if (count > 0)
	{
		centerV /= count;
		XMFLOAT3 center;
		XMStoreFloat3(&center, centerV);
		transform.ClearTransform();
		transform.Translate(center);
		transform.UpdateTransform();
	}

	// translator "bind matrix"
	XMMATRIX B = XMMatrixInverse(nullptr, XMLoadFloat4x4(&transform.world));

	for (auto& x : selected)
	{
		TransformComponent* transform_selected = scene.transforms.GetComponent(x.entity);
		if (transform_selected != nullptr)
		{
			// selected to world space:
			transform_selected->ApplyTransform();

			// selected to translator local space:
			transform_selected->MatrixTransform(B);
		}
	}
}
void Translator::PostTranslate()
{
	Scene& scene = wiScene::GetScene();

	for (auto& x : selected)
	{
		TransformComponent* transform_selected = scene.transforms.GetComponent(x.entity);
		if (transform_selected != nullptr)
		{
			XMFLOAT4X4 worldPrev = transform_selected->world;

			transform_selected->UpdateTransform_Parented(transform);

			// selected to world space:
			transform_selected->ApplyTransform();

			// selected to parent local space (if has parent):
			const HierarchyComponent* hier = scene.hierarchy.GetComponent(x.entity);
			if (hier != nullptr)
			{
				const TransformComponent* transform_parent = scene.transforms.GetComponent(hier->parentID);
				if (transform_parent != nullptr)
				{
					transform_selected->MatrixTransform(XMMatrixInverse(nullptr, XMLoadFloat4x4(&transform_parent->world)));
				}
			}

			// Restore worldPrev, so velocity buffer is correctly updated:
			//	(Only when dragging, otherwise if it's released, we want to record the matrix properly for undo)
			if (dragging)
			{
				transform_selected->world = worldPrev;
			}
		}
	}
}

