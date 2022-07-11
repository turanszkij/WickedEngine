#include "stdafx.h"
#include "Translator.h"
#include "wiRenderer.h"
#include "wiInput.h"
#include "wiMath.h"
#include "shaders/ShaderInterop_Renderer.h"
#include "wiEventHandler.h"

using namespace wi::ecs;
using namespace wi::scene;
using namespace wi::graphics;
using namespace wi::primitive;

namespace Translator_Internal
{
	PipelineState pso_solidpart;
	PipelineState pso_wirepart;
	const float origin_size = 0.2f;
	const float axis_length = 3.5f;
	const float plane_min = 0.5f;
	const float plane_max = 1.5f;
	const float circle_radius = axis_length;
	const float circle_width = 0.75f;
	const float circle2_radius = circle_radius + 0.7f;
	const float circle2_width = 0.3f;

	void LoadShaders()
	{
		GraphicsDevice* device = wi::graphics::GetDevice();

		{
			PipelineStateDesc desc;

			desc.vs = wi::renderer::GetShader(wi::enums::VSTYPE_VERTEXCOLOR);
			desc.ps = wi::renderer::GetShader(wi::enums::PSTYPE_VERTEXCOLOR);
			desc.il = wi::renderer::GetInputLayout(wi::enums::ILTYPE_VERTEXCOLOR);
			desc.dss = wi::renderer::GetDepthStencilState(wi::enums::DSSTYPE_DEFAULT);
			desc.rs = wi::renderer::GetRasterizerState(wi::enums::RSTYPE_DOUBLESIDED);
			desc.bs = wi::renderer::GetBlendState(wi::enums::BSTYPE_TRANSPARENT);
			desc.pt = PrimitiveTopology::TRIANGLELIST;

			device->CreatePipelineState(&desc, &pso_solidpart);
		}

		{
			PipelineStateDesc desc;

			desc.vs = wi::renderer::GetShader(wi::enums::VSTYPE_VERTEXCOLOR);
			desc.ps = wi::renderer::GetShader(wi::enums::PSTYPE_VERTEXCOLOR);
			desc.il = wi::renderer::GetInputLayout(wi::enums::ILTYPE_VERTEXCOLOR);
			desc.dss = wi::renderer::GetDepthStencilState(wi::enums::DSSTYPE_DEFAULT);
			desc.rs = wi::renderer::GetRasterizerState(wi::enums::RSTYPE_WIRE_DOUBLESIDED_SMOOTH);
			desc.bs = wi::renderer::GetBlendState(wi::enums::BSTYPE_TRANSPARENT);
			desc.pt = PrimitiveTopology::LINELIST;

			device->CreatePipelineState(&desc, &pso_wirepart);
		}
	}


	struct Vertex
	{
		XMFLOAT4 position;
		XMFLOAT4 color;
	};
	const Vertex cubeVerts[] = {
		{XMFLOAT4(-1,1,1,1),   XMFLOAT4(1,1,1,1)},
		{XMFLOAT4(-1,-1,1,1),  XMFLOAT4(1,1,1,1)},
		{XMFLOAT4(-1,-1,-1,1), XMFLOAT4(1,1,1,1)},
		{XMFLOAT4(1,1,1,1),	XMFLOAT4(1,1,1,1)},
		{XMFLOAT4(1,-1,1,1),   XMFLOAT4(1,1,1,1)},
		{XMFLOAT4(-1,-1,1,1),  XMFLOAT4(1,1,1,1)},
		{XMFLOAT4(1,1,-1,1),   XMFLOAT4(1,1,1,1)},
		{XMFLOAT4(1,-1,-1,1),  XMFLOAT4(1,1,1,1)},
		{XMFLOAT4(1,-1,1,1),   XMFLOAT4(1,1,1,1)},
		{XMFLOAT4(-1,1,-1,1),  XMFLOAT4(1,1,1,1)},
		{XMFLOAT4(-1,-1,-1,1), XMFLOAT4(1,1,1,1)},
		{XMFLOAT4(1,-1,-1,1),  XMFLOAT4(1,1,1,1)},
		{XMFLOAT4(-1,-1,1,1),  XMFLOAT4(1,1,1,1)},
		{XMFLOAT4(1,-1,1,1),   XMFLOAT4(1,1,1,1)},
		{XMFLOAT4(1,-1,-1,1),  XMFLOAT4(1,1,1,1)},
		{XMFLOAT4(1,1,1,1),	XMFLOAT4(1,1,1,1)},
		{XMFLOAT4(-1,1,1,1),   XMFLOAT4(1,1,1,1)},
		{XMFLOAT4(-1,1,-1,1),  XMFLOAT4(1,1,1,1)},
		{XMFLOAT4(-1,1,-1,1),  XMFLOAT4(1,1,1,1)},
		{XMFLOAT4(-1,1,1,1),   XMFLOAT4(1,1,1,1)},
		{XMFLOAT4(-1,-1,-1,1), XMFLOAT4(1,1,1,1)},
		{XMFLOAT4(-1,1,1,1),   XMFLOAT4(1,1,1,1)},
		{XMFLOAT4(1,1,1,1),	XMFLOAT4(1,1,1,1)},
		{XMFLOAT4(-1,-1,1,1),  XMFLOAT4(1,1,1,1)},
		{XMFLOAT4(1,1,1,1),	XMFLOAT4(1,1,1,1)},
		{XMFLOAT4(1,1,-1,1),   XMFLOAT4(1,1,1,1)},
		{XMFLOAT4(1,-1,1,1),   XMFLOAT4(1,1,1,1)},
		{XMFLOAT4(1,1,-1,1),   XMFLOAT4(1,1,1,1)},
		{XMFLOAT4(-1,1,-1,1),  XMFLOAT4(1,1,1,1)},
		{XMFLOAT4(1,-1,-1,1),  XMFLOAT4(1,1,1,1)},
		{XMFLOAT4(-1,-1,-1,1), XMFLOAT4(1,1,1,1)},
		{XMFLOAT4(-1,-1,1,1),  XMFLOAT4(1,1,1,1)},
		{XMFLOAT4(1,-1,-1,1),  XMFLOAT4(1,1,1,1)},
		{XMFLOAT4(1,1,-1,1),   XMFLOAT4(1,1,1,1)},
		{XMFLOAT4(1,1,1,1),	XMFLOAT4(1,1,1,1)},
		{XMFLOAT4(-1,1,-1,1),  XMFLOAT4(1,1,1,1)},
	};
}
using namespace Translator_Internal;

void Translator::Update(const wi::Canvas& canvas)
{
	if (selected.empty())
	{
		transform.ClearTransform();
		transform.UpdateTransform();
		return;
	}

	dragStarted = false;
	dragEnded = false;

	XMFLOAT4 pointer = wi::input::GetPointer();
	const CameraComponent& cam = wi::scene::GetCamera();
	XMVECTOR pos = transform.GetPositionV();

	// Non recursive selection will be computed to not apply recursive operations two times
	//	A recursive operation is for example translating a parent transform
	//	An other recursive operation is serializing selected parent entities
	Scene& scene = wi::scene::GetScene();
	selectedEntitiesLookup.clear();
	for (auto& x : selected)
	{
		selectedEntitiesLookup.insert(x.entity);
	}
	selectedEntitiesNonRecursive.clear();
	for (auto& x : selected)
	{
		const HierarchyComponent* hier = scene.hierarchy.GetComponent(x.entity);
		if (hier == nullptr || selectedEntitiesLookup.count(hier->parentID) == 0)
		{
			selectedEntitiesNonRecursive.push_back(x.entity);
		}
	}

	if (enabled)
	{
		PreTranslate();

		const Ray ray = wi::renderer::GetPickRay((long)pointer.x, (long)pointer.y, canvas, cam);
		const XMVECTOR rayOrigin = XMLoadFloat3(&ray.origin);
		const XMVECTOR rayDir = XMLoadFloat3(&ray.direction);

		if (!dragging)
		{
			// Decide which state to enter for dragging:
			XMMATRIX P = cam.GetProjection();
			XMMATRIX V = cam.GetView();
			XMMATRIX W = XMMatrixIdentity();
			XMFLOAT3 p = transform.GetPosition();

			dist = std::max(wi::math::Distance(p, cam.Eye) * 0.05f, 0.0001f);

			if (isRotator)
			{
				XMVECTOR plane_zy = XMPlaneFromPointNormal(pos, XMVectorSet(1, 0, 0, 0));
				XMVECTOR plane_xz = XMPlaneFromPointNormal(pos, XMVectorSet(0, 1, 0, 0));
				XMVECTOR plane_xy = XMPlaneFromPointNormal(pos, XMVectorSet(0, 0, 1, 0));

				XMVECTOR intersection = XMPlaneIntersectLine(plane_zy, rayOrigin, rayOrigin + rayDir * cam.zFarP);
				float dist_x = XMVectorGetX(XMVector3LengthSq(intersection - rayOrigin));
				float len_x = XMVectorGetX(XMVector3Length(intersection - pos)) / dist;
				intersection = XMPlaneIntersectLine(plane_xz, rayOrigin, rayOrigin + rayDir * cam.zFarP);
				float dist_y = XMVectorGetX(XMVector3LengthSq(intersection - rayOrigin));
				float len_y = XMVectorGetX(XMVector3Length(intersection - pos)) / dist;
				intersection = XMPlaneIntersectLine(plane_xy, rayOrigin, rayOrigin + rayDir * cam.zFarP);
				float dist_z = XMVectorGetX(XMVector3LengthSq(intersection - rayOrigin));
				float len_z = XMVectorGetX(XMVector3Length(intersection - pos)) / dist;

				state = TRANSLATOR_IDLE;

				float range = circle_width * 0.5f;
				float perimeter = circle_radius - range;
				float best_dist = std::numeric_limits<float>::max();
				if (std::abs(perimeter - len_x) <= range && dist_x < best_dist)
				{
					state = TRANSLATOR_X;
					axis = XMFLOAT3(1, 0, 0);
					best_dist = dist_x;
				}
				if (std::abs(perimeter - len_y) <= range && dist_y < best_dist)
				{
					state = TRANSLATOR_Y;
					axis = XMFLOAT3(0, 1, 0);
					best_dist = dist_y;
				}
				if (std::abs(perimeter - len_z) <= range && dist_z < best_dist)
				{
					state = TRANSLATOR_Z;
					axis = XMFLOAT3(0, 0, 1);
					best_dist = dist_z;
				}

				XMVECTOR screen_normal = XMVector3Normalize(cam.GetEye() - pos);
				XMVECTOR plane_screen = XMPlaneFromPointNormal(pos, screen_normal);
				intersection = XMPlaneIntersectLine(plane_screen, rayOrigin, rayOrigin + rayDir * cam.zFarP);
				float len_screen = XMVectorGetX(XMVector3Length(intersection - pos)) / dist;
				range = circle2_width * 0.5f;
				perimeter = circle2_radius - range;
				if (std::abs(perimeter - len_screen) <= range)
				{
					state = TRANSLATOR_XYZ;
					XMStoreFloat3(&axis, screen_normal);
				}
				
			}
			else
			{
				AABB aabb_origin;
				aabb_origin.createFromHalfWidth(p, XMFLOAT3(origin_size * dist, origin_size * dist, origin_size * dist));

				XMFLOAT3 maxp;
				XMStoreFloat3(&maxp, pos + XMVectorSet(axis_length, 0, 0, 0) * dist);
				AABB aabb_x = AABB::Merge(AABB(p, maxp), aabb_origin);

				XMStoreFloat3(&maxp, pos + XMVectorSet(0, axis_length, 0, 0) * dist);
				AABB aabb_y = AABB::Merge(AABB(p, maxp), aabb_origin);

				XMStoreFloat3(&maxp, pos + XMVectorSet(0, 0, axis_length, 0) * dist);
				AABB aabb_z = AABB::Merge(AABB(p, maxp), aabb_origin);

				XMFLOAT3 minp;
				XMStoreFloat3(&minp, pos + XMVectorSet(plane_min, plane_min, 0, 0) * dist);
				XMStoreFloat3(&maxp, pos + XMVectorSet(plane_max, plane_max, 0, 0) * dist);
				AABB aabb_xy = AABB(minp, maxp);

				XMStoreFloat3(&minp, pos + XMVectorSet(plane_min, 0, plane_min, 0) * dist);
				XMStoreFloat3(&maxp, pos + XMVectorSet(plane_max, 0, plane_max, 0) * dist);
				AABB aabb_xz = AABB(minp, maxp);

				XMStoreFloat3(&minp, pos + XMVectorSet(0, plane_min, plane_min, 0) * dist);
				XMStoreFloat3(&maxp, pos + XMVectorSet(0, plane_max, plane_max, 0) * dist);
				AABB aabb_yz = AABB(minp, maxp);

				if (!isRotator && aabb_origin.intersects(ray))
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
					XMVECTOR N = XMVectorSet(0, 0, 1, 0);

					float prio = FLT_MAX;
					if (!isRotator && !isScalator && aabb_xy.intersects(ray))
					{
						state = TRANSLATOR_XY;
						prio = XMVectorGetX(XMVector3Dot(N, (rayOrigin - pos) / XMVectorAbs(XMVector3Dot(N, rayDir))));
					}

					N = XMVectorSet(0, 1, 0, 0);
					float d = XMVectorGetX(XMVector3Dot(N, (rayOrigin - pos) / XMVectorAbs(XMVector3Dot(N, rayDir))));
					if (!isRotator && !isScalator && d < prio && aabb_xz.intersects(ray))
					{
						state = TRANSLATOR_XZ;
						prio = d;
					}

					N = XMVectorSet(1, 0, 0, 0);
					d = XMVectorGetX(XMVector3Dot(N, (rayOrigin - pos) / XMVectorAbs(XMVector3Dot(N, rayDir))));
					if (!isRotator && !isScalator && d < prio && aabb_yz.intersects(ray))
					{
						state = TRANSLATOR_YZ;
					}
				}
			}
		}

		if (dragging || (state != TRANSLATOR_IDLE && wi::input::Press(wi::input::MOUSE_BUTTON_LEFT)))
		{
			// Dragging operation:
			if (isRotator)
			{
				XMVECTOR intersection = XMPlaneIntersectLine(XMPlaneFromPointNormal(pos, XMLoadFloat3(&axis)), rayOrigin, rayOrigin + rayDir * cam.zFarP);

				if (!dragging)
				{
					dragStarted = true;
					transform_start = transform;
					XMStoreFloat3(&intersection_start, intersection);
					angle_prev = 0;
				}
				XMVECTOR intersectionPrev = XMLoadFloat3(&intersection_start);

				XMVECTOR o = XMVector3Normalize(intersectionPrev - pos);
				XMVECTOR c = XMVector3Normalize(intersection - pos);
				XMFLOAT3 original, current;
				XMStoreFloat3(&original, o);
				XMStoreFloat3(&current, c);
				angle = wi::math::GetAngle(original, current, axis);

				switch (state)
				{
				case Translator::TRANSLATOR_X:
					angle_start = wi::math::GetAngle(XMFLOAT3(0, 1, 0), original, axis);
					break;
				case Translator::TRANSLATOR_Y:
					angle_start = wi::math::GetAngle(XMFLOAT3(0, 0, 1), original, axis);
					break;
				case Translator::TRANSLATOR_Z:
					angle_start = wi::math::GetAngle(XMFLOAT3(1, 0, 0), original, axis);
					break;
				case Translator::TRANSLATOR_XYZ:
					{
						XMMATRIX M = XMMatrixInverse(nullptr, XMMatrixLookToLH(XMVectorZero(), XMVector3Normalize(transform.GetPositionV() - cam.GetEye()), cam.GetUp()));
						XMFLOAT3 ref;
						XMStoreFloat3(&ref, XMVector3TransformNormal(XMVectorSet(0, 1, 0, 0), M));
						angle_start = wi::math::GetAngle(ref, original, axis);
					}
					break;
				default:
					break;
				}

				transform.Rotate(XMQuaternionRotationAxis(XMLoadFloat3(&axis), angle - angle_prev));
				angle_prev = angle;
			}
			else
			{
				XMVECTOR plane, planeNormal;
				if (state == TRANSLATOR_X)
				{
					XMVECTOR axis = XMVectorSet(1, 0, 0, 0);
					XMVECTOR wrong = XMVector3Cross(cam.GetAt(), axis);
					planeNormal = XMVector3Cross(wrong, axis);
					this->axis = XMFLOAT3(1, 0, 0);
				}
				else if (state == TRANSLATOR_Y)
				{
					XMVECTOR axis = XMVectorSet(0, 1, 0, 0);
					XMVECTOR wrong = XMVector3Cross(cam.GetAt(), axis);
					planeNormal = XMVector3Cross(wrong, axis);
					this->axis = XMFLOAT3(0, 1, 0);
				}
				else if (state == TRANSLATOR_Z)
				{
					XMVECTOR axis = XMVectorSet(0, 0, 1, 0);
					XMVECTOR wrong = XMVector3Cross(cam.GetUp(), axis);
					planeNormal = XMVector3Cross(wrong, axis);
					this->axis = XMFLOAT3(0, 0, 1);
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

				if (XMVectorGetX(XMVectorAbs(XMVector3Dot(planeNormal, rayDir))) < 0.001f)
					return;
				XMVECTOR intersection = XMPlaneIntersectLine(plane, rayOrigin, rayOrigin + rayDir * cam.zFarP);

				if (!dragging)
				{
					dragStarted = true;
					transform_start = transform;
					XMStoreFloat3(&intersection_start, intersection);
				}
				XMVECTOR intersectionPrev = XMLoadFloat3(&intersection_start);

				XMVECTOR deltaV;
				if (state == TRANSLATOR_X)
				{
					XMVECTOR A = pos, B = pos + XMVectorSet(1, 0, 0, 0);
					XMVECTOR P = wi::math::GetClosestPointToLine(A, B, intersection);
					XMVECTOR PPrev = wi::math::GetClosestPointToLine(A, B, intersectionPrev);
					deltaV = P - PPrev;
				}
				else if (state == TRANSLATOR_Y)
				{
					XMVECTOR A = pos, B = pos + XMVectorSet(0, 1, 0, 0);
					XMVECTOR P = wi::math::GetClosestPointToLine(A, B, intersection);
					XMVECTOR PPrev = wi::math::GetClosestPointToLine(A, B, intersectionPrev);
					deltaV = P - PPrev;
				}
				else if (state == TRANSLATOR_Z)
				{
					XMVECTOR A = pos, B = pos + XMVectorSet(0, 0, 1, 0);
					XMVECTOR P = wi::math::GetClosestPointToLine(A, B, intersection);
					XMVECTOR PPrev = wi::math::GetClosestPointToLine(A, B, intersectionPrev);
					deltaV = P - PPrev;
				}
				else
				{
					deltaV = intersection - intersectionPrev;

					if (isScalator)
					{
						deltaV = XMVectorReplicate(XMVectorGetY(deltaV));
					}
				}

				transform = transform_start;
				if (isTranslator)
				{
					transform.Translate(deltaV);
				}
				if (isScalator)
				{
					XMFLOAT3 delta;
					XMStoreFloat3(&delta, deltaV);
					XMFLOAT3 scale = transform.GetScale();
					scale = XMFLOAT3((1.0f / scale.x) * (scale.x + delta.x), (1.0f / scale.y) * (scale.y + delta.y), (1.0f / scale.z) * (scale.z + delta.z));
					transform.Scale(scale);
				}
			}

			if (wi::input::Down(wi::input::BUTTON::KEYBOARD_BUTTON_LCONTROL))
			{
				if (isTranslator)
				{
					transform.translation_local.x = std::round(transform.translation_local.x);
					transform.translation_local.y = std::round(transform.translation_local.y);
					transform.translation_local.z = std::round(transform.translation_local.z);
				}
				if (isRotator)
				{
					XMFLOAT3 euler = wi::math::QuaternionToRollPitchYaw(transform.rotation_local);
					euler.x = std::floor(euler.x / XM_PIDIV4) * XM_PIDIV4;
					euler.y = std::floor(euler.y / XM_PIDIV4) * XM_PIDIV4;
					euler.z = std::floor(euler.z / XM_PIDIV4) * XM_PIDIV4;
					XMVECTOR Q = XMQuaternionRotationRollPitchYaw(euler.x, euler.y, euler.z);
					Q = XMQuaternionNormalize(Q);
					XMStoreFloat4(&transform.rotation_local, Q);
				}
				if (isScalator)
				{
					transform.scale_local.x = std::round(transform.scale_local.x);
					transform.scale_local.y = std::round(transform.scale_local.y);
					transform.scale_local.z = std::round(transform.scale_local.z);
				}
			}

			if (isScalator)
			{
				transform.scale_local.x = std::max(0.001f, transform.scale_local.x);
				transform.scale_local.y = std::max(0.001f, transform.scale_local.y);
				transform.scale_local.z = std::max(0.001f, transform.scale_local.z);
			}

			transform.UpdateTransform();

			dragging = true;
		}

		if (!wi::input::Down(wi::input::MOUSE_BUTTON_LEFT))
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
}
void Translator::Draw(const CameraComponent& camera, CommandList cmd) const
{
	if (!enabled || selected.empty())
	{
		return;
	}

	static bool shaders_loaded = false;
	if (!shaders_loaded)
	{
		shaders_loaded = true;
		static wi::eventhandler::Handle handle = wi::eventhandler::Subscribe(wi::eventhandler::EVENT_RELOAD_SHADERS, [](uint64_t userdata) { Translator_Internal::LoadShaders(); });
		Translator_Internal::LoadShaders();
	}

	Scene& scene = wi::scene::GetScene();

	GraphicsDevice* device = wi::graphics::GetDevice();

	device->EventBegin("Translator", cmd);

	CameraComponent cam_tmp = camera;
	cam_tmp.jitter = XMFLOAT2(0, 0); // remove temporal jitter
	cam_tmp.UpdateCamera();
	XMMATRIX VP = cam_tmp.GetViewProjection();

	MiscCB sb;

	XMMATRIX mat = XMMatrixScaling(dist, dist, dist)*XMMatrixTranslationFromVector(transform.GetPositionV()) * VP;
	XMMATRIX matX = mat;
	XMMATRIX matY = XMMatrixRotationZ(XM_PIDIV2)*XMMatrixRotationY(XM_PIDIV2)*mat;
	XMMATRIX matZ = XMMatrixRotationY(-XM_PIDIV2)*XMMatrixRotationZ(-XM_PIDIV2)*mat;

	const float channel_min = 0.25f; // min color channel, to avoid pure red/green/blue
	const XMFLOAT4 highlight_color = XMFLOAT4(1, 0.6f, 0, 1);

	// Axes:
	{
		device->BindPipelineState(&pso_solidpart, cmd);

		uint32_t vertexCount = 0;
		GraphicsDevice::GPUAllocation mem;

		if (isRotator)
		{
			const uint32_t segmentCount = 90;
			const uint32_t circle_triangleCount = segmentCount * 2;
			vertexCount = circle_triangleCount * 3;
			mem = device->AllocateGPU(sizeof(Vertex) * vertexCount, cmd);
			uint8_t* dst = (uint8_t*)mem.data;
			for (uint32_t i = 0; i < segmentCount; ++i)
			{
				const float angle0 = (float)i / (float)segmentCount * XM_2PI;
				const float angle1 = (float)(i + 1) / (float)segmentCount * XM_2PI;

				// circle:
				const float circle_radius_inner = circle_radius - circle_width;
				const float circle_halfway = circle_radius - circle_width * 0.5f;
				const Vertex verts[] = {
					{XMFLOAT4(0, std::sin(angle0) * circle_radius_inner, std::cos(angle0) * circle_radius_inner, 1), XMFLOAT4(1,1,1,1)},
					{XMFLOAT4(0, std::sin(angle1) * circle_radius_inner, std::cos(angle1) * circle_radius_inner, 1), XMFLOAT4(1,1,1,1)},
					{XMFLOAT4(0, std::sin(angle0) * circle_radius, std::cos(angle0) * circle_radius, 1), XMFLOAT4(1,1,1,1)},
					{XMFLOAT4(0, std::sin(angle0) * circle_radius, std::cos(angle0) * circle_radius, 1), XMFLOAT4(1,1,1,1)},
					{XMFLOAT4(0, std::sin(angle1) * circle_radius, std::cos(angle1) * circle_radius, 1), XMFLOAT4(1,1,1,1)},
					{XMFLOAT4(0, std::sin(angle1) * circle_radius_inner, std::cos(angle1) * circle_radius_inner, 1), XMFLOAT4(1,1,1,1)},
				};
				std::memcpy(dst, verts, sizeof(verts));
				dst += sizeof(verts);
			}
		}
		else
		{
			const uint32_t segmentCount = 18;
			const uint32_t cylinder_triangleCount = segmentCount * 2;
			const uint32_t cone_triangleCount = cylinder_triangleCount;
			if (isTranslator)
			{
				vertexCount = (cylinder_triangleCount + cone_triangleCount) * 3;
			}
			else if (isScalator)
			{
				vertexCount = cylinder_triangleCount * 3 + arraysize(cubeVerts);
			}
			mem = device->AllocateGPU(sizeof(Vertex) * vertexCount, cmd);

			const float cone_length = 0.75f;
			float cylinder_length = axis_length;
			if (isTranslator)
			{
				cylinder_length -= cone_length;
			}
			uint8_t* dst = (uint8_t*)mem.data;
			for (uint32_t i = 0; i < segmentCount; ++i)
			{
				const float angle0 = (float)i / (float)segmentCount * XM_2PI;
				const float angle1 = (float)(i + 1) / (float)segmentCount * XM_2PI;
				// cylinder base:
				{
					const float cylinder_radius = 0.075f;
					const Vertex verts[] = {
						{XMFLOAT4(origin_size, std::sin(angle0) * cylinder_radius, std::cos(angle0) * cylinder_radius, 1), XMFLOAT4(1,1,1,1)},
						{XMFLOAT4(origin_size, std::sin(angle1) * cylinder_radius, std::cos(angle1) * cylinder_radius, 1), XMFLOAT4(1,1,1,1)},
						{XMFLOAT4(cylinder_length, std::sin(angle0) * cylinder_radius, std::cos(angle0) * cylinder_radius, 1), XMFLOAT4(1,1,1,1)},
						{XMFLOAT4(cylinder_length, std::sin(angle0) * cylinder_radius, std::cos(angle0) * cylinder_radius, 1), XMFLOAT4(1,1,1,1)},
						{XMFLOAT4(cylinder_length, std::sin(angle1) * cylinder_radius, std::cos(angle1) * cylinder_radius, 1), XMFLOAT4(1,1,1,1)},
						{XMFLOAT4(origin_size, std::sin(angle1) * cylinder_radius, std::cos(angle1) * cylinder_radius, 1), XMFLOAT4(1,1,1,1)},
					};
					std::memcpy(dst, verts, sizeof(verts));
					dst += sizeof(verts);
				}
				if (isTranslator)
				{
					// cone cap:
					const float cone_radius = origin_size;
					const Vertex verts[] = {
						{XMFLOAT4(cylinder_length, 0, 0, 1), XMFLOAT4(1,1,1,1)},
						{XMFLOAT4(cylinder_length, std::sin(angle0) * cone_radius, std::cos(angle0) * cone_radius, 1), XMFLOAT4(1,1,1,1)},
						{XMFLOAT4(cylinder_length, std::sin(angle1) * cone_radius, std::cos(angle1) * cone_radius, 1), XMFLOAT4(1,1,1,1)},
						{XMFLOAT4(axis_length, 0, 0, 1), XMFLOAT4(1,1,1,1)},
						{XMFLOAT4(cylinder_length, std::sin(angle0) * cone_radius, std::cos(angle0) * cone_radius, 1), XMFLOAT4(1,1,1,1)},
						{XMFLOAT4(cylinder_length, std::sin(angle1) * cone_radius, std::cos(angle1) * cone_radius, 1), XMFLOAT4(1,1,1,1)},
					};
					std::memcpy(dst, verts, sizeof(verts));
					dst += sizeof(verts);
				}
			}

			if (isScalator)
			{
				// cube cap:
				for (uint32_t i = 0; i < arraysize(cubeVerts); ++i)
				{
					Vertex vert = cubeVerts[i];
					vert.position.x = vert.position.x * origin_size + cylinder_length - origin_size;
					vert.position.y = vert.position.y * origin_size;
					vert.position.z = vert.position.z * origin_size;
					std::memcpy(dst, &vert, sizeof(vert));
					dst += sizeof(vert);
				}
			}
		}

		const GPUBuffer* vbs[] = {
			&mem.buffer,
		};
		const uint32_t strides[] = {
			sizeof(Vertex),
		};
		const uint64_t offsets[] = {
			mem.offset,
		};
		device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, offsets, cmd);

		// x
		XMStoreFloat4x4(&sb.g_xTransform, matX);
		sb.g_xColor = state == TRANSLATOR_X ? highlight_color : XMFLOAT4(1, channel_min, channel_min, 1);
		device->BindDynamicConstantBuffer(sb, CBSLOT_RENDERER_MISC, cmd);
		device->Draw(vertexCount, 0, cmd);

		// y
		XMStoreFloat4x4(&sb.g_xTransform, matY);
		sb.g_xColor = state == TRANSLATOR_Y ? highlight_color : XMFLOAT4(channel_min, 1, channel_min, 1);
		device->BindDynamicConstantBuffer(sb, CBSLOT_RENDERER_MISC, cmd);
		device->Draw(vertexCount, 0, cmd);

		// z
		XMStoreFloat4x4(&sb.g_xTransform, matZ);
		sb.g_xColor = state == TRANSLATOR_Z ? highlight_color : XMFLOAT4(channel_min, channel_min, 1, 1);
		device->BindDynamicConstantBuffer(sb, CBSLOT_RENDERER_MISC, cmd);
		device->Draw(vertexCount, 0, cmd);

	}

	if (isRotator)
	{
		// An other circle for rotator, a bit thinner and screen facing, so new geo:
		const uint32_t segmentCount = 90;
		const uint32_t circle2_triangleCount = segmentCount * 2;
		uint32_t vertexCount = circle2_triangleCount * 3;
		GraphicsDevice::GPUAllocation mem = device->AllocateGPU(sizeof(Vertex) * vertexCount, cmd);
		uint8_t* dst = (uint8_t*)mem.data;
		for (uint32_t i = 0; i < segmentCount; ++i)
		{
			const float angle0 = (float)i / (float)segmentCount * XM_2PI;
			const float angle1 = (float)(i + 1) / (float)segmentCount * XM_2PI;

			// circle:
			const float circle2_radius_inner = circle2_radius - circle2_width;
			const float circle2_halfway = circle2_radius - circle2_width * 0.5f;
			const Vertex verts[] = {
				{XMFLOAT4(0, std::sin(angle0) * circle2_radius_inner, std::cos(angle0) * circle2_radius_inner, 1), XMFLOAT4(1,1,1,1)},
				{XMFLOAT4(0, std::sin(angle1) * circle2_radius_inner, std::cos(angle1) * circle2_radius_inner, 1), XMFLOAT4(1,1,1,1)},
				{XMFLOAT4(0, std::sin(angle0) * circle2_radius, std::cos(angle0) * circle2_radius, 1), XMFLOAT4(1,1,1,1)},
				{XMFLOAT4(0, std::sin(angle0) * circle2_radius, std::cos(angle0) * circle2_radius, 1), XMFLOAT4(1,1,1,1)},
				{XMFLOAT4(0, std::sin(angle1) * circle2_radius, std::cos(angle1) * circle2_radius, 1), XMFLOAT4(1,1,1,1)},
				{XMFLOAT4(0, std::sin(angle1) * circle2_radius_inner, std::cos(angle1) * circle2_radius_inner, 1), XMFLOAT4(1,1,1,1)},
			};
			std::memcpy(dst, verts, sizeof(verts));
			dst += sizeof(verts);
		}

		const GPUBuffer* vbs[] = {
			&mem.buffer,
		};
		const uint32_t strides[] = {
			sizeof(Vertex),
		};
		const uint64_t offsets[] = {
			mem.offset,
		};
		device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, offsets, cmd);

		XMStoreFloat4x4(&sb.g_xTransform,
			XMMatrixRotationY(XM_PIDIV2) *
			XMMatrixInverse(nullptr, XMMatrixLookToLH(XMVectorZero(), XMVector3Normalize(transform.GetPositionV() - camera.GetEye()), camera.GetUp())) *
			mat
		);
		sb.g_xColor = state == TRANSLATOR_XYZ ? highlight_color : XMFLOAT4(1, 1, 1, 0.5f);
		device->BindDynamicConstantBuffer(sb, CBSLOT_RENDERER_MISC, cmd);
		device->Draw(vertexCount, 0, cmd);
	}

	// Origin:
	if(!isRotator)
	{
		device->BindPipelineState(&pso_solidpart, cmd);

		GraphicsDevice::GPUAllocation mem = device->AllocateGPU(sizeof(cubeVerts), cmd);
		std::memcpy(mem.data, cubeVerts, sizeof(cubeVerts));
		const GPUBuffer* vbs[] = {
			&mem.buffer,
		};
		const uint32_t strides[] = {
			sizeof(Vertex),
		};
		const uint64_t offsets[] = {
			mem.offset,
		};
		device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, offsets, cmd);

		XMStoreFloat4x4(&sb.g_xTransform, XMMatrixScaling(origin_size, origin_size, origin_size) * mat);
		sb.g_xColor = state == TRANSLATOR_XYZ ? highlight_color : XMFLOAT4(0.5f, 0.5f, 0.5f, 1);
		device->BindDynamicConstantBuffer(sb, CBSLOT_RENDERER_MISC, cmd);
		device->Draw(arraysize(cubeVerts), 0, cmd);
	}

	// Planes:
	if (!isRotator && !isScalator)
	{
		// Wire part:
		{
			device->BindPipelineState(&pso_wirepart, cmd);

			const Vertex verts[] = {
				{XMFLOAT4(plane_min,plane_min,0,1), XMFLOAT4(1,1,1,1)},
				{XMFLOAT4(plane_min,plane_max,0,1), XMFLOAT4(1,1,1,1)},

				{XMFLOAT4(plane_min,plane_max,0,1), XMFLOAT4(1,1,1,1)},
				{XMFLOAT4(plane_max,plane_max,0,1), XMFLOAT4(1,1,1,1)},

				{XMFLOAT4(plane_max,plane_max,0,1), XMFLOAT4(1,1,1,1)},
				{XMFLOAT4(plane_max,plane_min,0,1), XMFLOAT4(1,1,1,1)},

				{XMFLOAT4(plane_max,plane_min,0,1), XMFLOAT4(1,1,1,1)},
				{XMFLOAT4(plane_min,plane_min,0,1), XMFLOAT4(1,1,1,1)},
			};
			GraphicsDevice::GPUAllocation mem = device->AllocateGPU(sizeof(verts), cmd);
			std::memcpy(mem.data, verts, sizeof(verts));
			const GPUBuffer* vbs[] = {
				&mem.buffer,
			};
			const uint32_t strides[] = {
				sizeof(Vertex),
			};
			const uint64_t offsets[] = {
				mem.offset,
			};
			device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, offsets, cmd);

			// xy
			XMStoreFloat4x4(&sb.g_xTransform, matX);
			sb.g_xColor = state == TRANSLATOR_XY ? highlight_color : XMFLOAT4(channel_min, channel_min, 1, 1);
			device->BindDynamicConstantBuffer(sb, CBSLOT_RENDERER_MISC, cmd);
			device->Draw(arraysize(verts), 0, cmd);

			// xz
			XMStoreFloat4x4(&sb.g_xTransform, matZ);
			sb.g_xColor = state == TRANSLATOR_XZ ? highlight_color : XMFLOAT4(channel_min, 1, channel_min, 1);
			device->BindDynamicConstantBuffer(sb, CBSLOT_RENDERER_MISC, cmd);
			device->Draw(arraysize(verts), 0, cmd);

			// yz
			XMStoreFloat4x4(&sb.g_xTransform, matY);
			sb.g_xColor = state == TRANSLATOR_YZ ? highlight_color : XMFLOAT4(1, channel_min, channel_min, 1);
			device->BindDynamicConstantBuffer(sb, CBSLOT_RENDERER_MISC, cmd);
			device->Draw(arraysize(verts), 0, cmd);
		}

		// Quad part:
		{
			device->BindPipelineState(&pso_solidpart, cmd);

			const Vertex verts[] = {
				{XMFLOAT4(plane_min,plane_min,0,1), XMFLOAT4(1,1,1,1)},
				{XMFLOAT4(plane_max,plane_min,0,1), XMFLOAT4(1,1,1,1)},
				{XMFLOAT4(plane_max,plane_max,0,1), XMFLOAT4(1,1,1,1)},

				{XMFLOAT4(plane_min,plane_min,0,1), XMFLOAT4(1,1,1,1)},
				{XMFLOAT4(plane_max,plane_max,0,1), XMFLOAT4(1,1,1,1)},
				{XMFLOAT4(plane_min,plane_max,0,1), XMFLOAT4(1,1,1,1)},
			};
			GraphicsDevice::GPUAllocation mem = device->AllocateGPU(sizeof(verts), cmd);
			std::memcpy(mem.data, verts, sizeof(verts));
			const GPUBuffer* vbs[] = {
				&mem.buffer,
			};
			const uint32_t strides[] = {
				sizeof(Vertex),
			};
			const uint64_t offsets[] = {
				mem.offset,
			};
			device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, offsets, cmd);

			// xy
			XMStoreFloat4x4(&sb.g_xTransform, matX);
			sb.g_xColor = state == TRANSLATOR_XY ? highlight_color : XMFLOAT4(channel_min, channel_min, 1, 0.25f);
			device->BindDynamicConstantBuffer(sb, CBSLOT_RENDERER_MISC, cmd);
			device->Draw(arraysize(verts), 0, cmd);

			// xz
			XMStoreFloat4x4(&sb.g_xTransform, matZ);
			sb.g_xColor = state == TRANSLATOR_XZ ? highlight_color : XMFLOAT4(channel_min, 1, channel_min, 0.25f);
			device->BindDynamicConstantBuffer(sb, CBSLOT_RENDERER_MISC, cmd);
			device->Draw(arraysize(verts), 0, cmd);

			// yz
			XMStoreFloat4x4(&sb.g_xTransform, matY);
			sb.g_xColor = state == TRANSLATOR_YZ ? highlight_color : XMFLOAT4(1, channel_min, channel_min, 0.25f);
			device->BindDynamicConstantBuffer(sb, CBSLOT_RENDERER_MISC, cmd);
			device->Draw(arraysize(verts), 0, cmd);
		}
	}


	// Dragging visualizer:
	if (dragging)
	{
		if (isTranslator)
		{
			device->BindPipelineState(&pso_wirepart, cmd);
			const Vertex verts[] = {
				{XMFLOAT4(transform_start.translation_local.x, transform_start.translation_local.y, transform_start.translation_local.z, 1), XMFLOAT4(1,1,1,1)},
				{XMFLOAT4(transform.translation_local.x, transform.translation_local.y, transform.translation_local.z, 1), XMFLOAT4(1,1,1,1)},
			};
			GraphicsDevice::GPUAllocation mem = device->AllocateGPU(sizeof(verts), cmd);
			std::memcpy(mem.data, verts, sizeof(verts));
			const GPUBuffer* vbs[] = {
				&mem.buffer,
			};
			const uint32_t strides[] = {
				sizeof(Vertex),
			};
			const uint64_t offsets[] = {
				mem.offset,
			};
			device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, offsets, cmd);

			XMStoreFloat4x4(&sb.g_xTransform, VP);
			sb.g_xColor = XMFLOAT4(1, 1, 1, 1);
			device->BindDynamicConstantBuffer(sb, CBSLOT_RENDERER_MISC, cmd);
			device->Draw(arraysize(verts), 0, cmd);
		}
		else if (isRotator)
		{
			device->BindPipelineState(&pso_solidpart, cmd);

			const uint32_t segmentCount = 90;
			const uint32_t vertexCount = segmentCount * 3;
			GraphicsDevice::GPUAllocation mem = device->AllocateGPU(sizeof(Vertex) * vertexCount, cmd);

			switch (state)
			{
			case Translator::TRANSLATOR_X:
				XMStoreFloat4x4(&sb.g_xTransform, matX);
				break;
			case Translator::TRANSLATOR_Y:
				XMStoreFloat4x4(&sb.g_xTransform, matY);
				break;
			case Translator::TRANSLATOR_Z:
				XMStoreFloat4x4(&sb.g_xTransform, matZ);
				break;
			case Translator::TRANSLATOR_XYZ:
				XMStoreFloat4x4(&sb.g_xTransform,
					XMMatrixRotationY(XM_PIDIV2) *
					XMMatrixInverse(nullptr, XMMatrixLookToLH(XMVectorZero(), XMVector3Normalize(transform.GetPositionV() - camera.GetEye()), camera.GetUp())) *
					mat
				);
				break;
			default:
				break;
			}

			uint8_t* dst = (uint8_t*)mem.data;
			for (uint32_t i = 0; i < segmentCount; ++i)
			{
				const float angle0 = (float)i / (float)segmentCount * angle + angle_start;
				const float angle1 = (float)(i + 1) / (float)segmentCount * angle + angle_start;

				const float radius = state == TRANSLATOR_XYZ ? (circle2_radius - circle2_width) : (circle_radius - circle_width);
				const Vertex verts[] = {
					{XMFLOAT4(0, 0, 0, 1), XMFLOAT4(1,1,1,1)},
					{XMFLOAT4(0, std::cos(angle0) * radius, std::sin(angle0) * radius, 1), XMFLOAT4(1,1,1,1)},
					{XMFLOAT4(0, std::cos(angle1) * radius, std::sin(angle1) * radius, 1), XMFLOAT4(1,1,1,1)},
				};
				std::memcpy(dst, verts, sizeof(verts));
				dst += sizeof(verts);
			}

			const GPUBuffer* vbs[] = {
				&mem.buffer,
			};
			const uint32_t strides[] = {
				sizeof(Vertex),
			};
			const uint64_t offsets[] = {
				mem.offset,
			};
			device->BindVertexBuffers(vbs, 0, arraysize(vbs), strides, offsets, cmd);

			sb.g_xColor = XMFLOAT4(1, 1, 1, 0.5f);
			device->BindDynamicConstantBuffer(sb, CBSLOT_RENDERER_MISC, cmd);
			device->Draw(vertexCount, 0, cmd);
		}

		XMFLOAT4 pointer = wi::input::GetPointer();
		wi::font::Params params;
		params.posX = pointer.x - 20;
		params.posY = pointer.y + 20;
		params.v_align = wi::font::WIFALIGN_TOP;
		params.h_align = wi::font::WIFALIGN_RIGHT;
		params.scaling = 0.8f;
		params.shadowColor = wi::Color::Black();
		std::string str;
		if (isTranslator)
		{
			str += "Offset = " + std::to_string(transform.translation_local.x - transform_start.translation_local.x) + ", " + std::to_string(transform.translation_local.y - transform_start.translation_local.y) + ", " + std::to_string(transform.translation_local.z - transform_start.translation_local.z);
		}
		if (isRotator)
		{
			switch (state)
			{
			case Translator::TRANSLATOR_X:
				str += "Axis = X";
				break;
			case Translator::TRANSLATOR_Y:
				str += "Axis = Y";
				break;
			case Translator::TRANSLATOR_Z:
				str += "Axis = Z";
				break;
			case Translator::TRANSLATOR_XYZ:
				str += "Axis = Screen";
				break;
			default:
				break;
			}
			str += "\nAngle = " + std::to_string(int(angle / XM_PI * 180)) + " degrees";
		}
		if (isScalator)
		{
			str += "Scaling = " + std::to_string(transform.scale_local.x / transform_start.scale_local.x) + ", " + std::to_string(transform.scale_local.y / transform_start.scale_local.y) + ", " + std::to_string(transform.scale_local.z / transform_start.scale_local.z);
		}
		wi::font::Draw(str, params, cmd);
	}

	device->EventEnd(cmd);
}

void Translator::PreTranslate()
{
	Scene& scene = wi::scene::GetScene();

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
		transform.translation_local = {};
		//transform.ClearTransform();
		transform.Translate(center);
		transform.UpdateTransform();
	}

	// translator "bind matrix"
	XMMATRIX B = XMMatrixInverse(nullptr, XMLoadFloat4x4(&transform.world));

	for (auto& x : selectedEntitiesNonRecursive)
	{
		TransformComponent* transform_selected = scene.transforms.GetComponent(x);
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
	Scene& scene = wi::scene::GetScene();

	for (auto& x : selectedEntitiesNonRecursive)
	{
		TransformComponent* transform_selected = scene.transforms.GetComponent(x);
		if (transform_selected != nullptr)
		{
			transform_selected->UpdateTransform_Parented(transform);

			// selected to world space:
			transform_selected->ApplyTransform();

			// selected to parent local space (if has parent):
			const HierarchyComponent* hier = scene.hierarchy.GetComponent(x);
			if (hier != nullptr)
			{
				const TransformComponent* transform_parent = scene.transforms.GetComponent(hier->parentID);
				if (transform_parent != nullptr)
				{
					transform_selected->MatrixTransform(XMMatrixInverse(nullptr, XMLoadFloat4x4(&transform_parent->world)));
				}
			}
		}
	}
}

