#include "stdafx.h"
#include "Editor.h"
#include "ObjectWindow.h"
#include "wiScene.h"

#include "xatlas.h"

#include <string>

using namespace wi::ecs;
using namespace wi::scene;


static void SetPixel(uint8_t *dest, int destWidth, int x, int y, const uint8_t *color)
{
	uint8_t *pixel = &dest[x * 4 + y * (destWidth * 4)];
	pixel[0] = color[0];
	pixel[1] = color[1];
	pixel[2] = color[2];
	pixel[3] = color[3];
}

// https://github.com/miloyip/line/blob/master/line_bresenham.c
static void RasterizeLine(uint8_t *dest, int destWidth, const int *p1, const int *p2, const uint8_t *color)
{
	const int dx = abs(p2[0] - p1[0]), sx = p1[0] < p2[0] ? 1 : -1;
	const int dy = abs(p2[1] - p1[1]), sy = p1[1] < p2[1] ? 1 : -1;
	int err = (dx > dy ? dx : -dy) / 2;
	int current[2];
	current[0] = p1[0];
	current[1] = p1[1];
	while (SetPixel(dest, destWidth, current[0], current[1], color), current[0] != p2[0] || current[1] != p2[1])
	{
		const int e2 = err;
		if (e2 > -dx) { err -= dy; current[0] += sx; }
		if (e2 < dy) { err += dx; current[1] += sy; }
	}
}

// https://github.com/ssloy/tinyrenderer/wiki/Lesson-2:-Triangle-rasterization-and-back-face-culling
static void RasterizeTriangle(uint8_t *dest, int destWidth, const int *t0, const int *t1, const int *t2, const uint8_t *color)
{
	if (t0[1] > t1[1]) std::swap(t0, t1);
	if (t0[1] > t2[1]) std::swap(t0, t2);
	if (t1[1] > t2[1]) std::swap(t1, t2);
	int total_height = t2[1] - t0[1];
	for (int i = 0; i < total_height; i++) {
		bool second_half = i > t1[1] - t0[1] || t1[1] == t0[1];
		int segment_height = second_half ? t2[1] - t1[1] : t1[1] - t0[1];
		float alpha = (float)i / total_height;
		float beta = (float)(i - (second_half ? t1[1] - t0[1] : 0)) / segment_height;
		int A[2], B[2];
		for (int j = 0; j < 2; j++) {
			A[j] = int(t0[j] + (t2[j] - t0[j]) * alpha);
			B[j] = int(second_half ? t1[j] + (t2[j] - t1[j]) * beta : t0[j] + (t1[j] - t0[j]) * beta);
		}
		if (A[0] > B[0]) std::swap(A, B);
		for (int j = A[0]; j <= B[0]; j++)
			SetPixel(dest, destWidth, j, t0[1] + i, color);
	}
}

struct Atlas_Dim
{
	uint32_t width = 0;
	uint32_t height = 0;
};
static Atlas_Dim GenerateMeshAtlas(MeshComponent& meshcomponent, uint32_t resolution)
{
	Atlas_Dim dim;

	xatlas::Atlas* atlas = xatlas::Create();

	// Prepare mesh to be processed by xatlas:
	{
		xatlas::MeshDecl mesh;
		mesh.vertexCount = (int)meshcomponent.vertex_positions.size();
		mesh.vertexPositionData = meshcomponent.vertex_positions.data();
		mesh.vertexPositionStride = sizeof(float) * 3;
		if (!meshcomponent.vertex_normals.empty()) {
			mesh.vertexNormalData = meshcomponent.vertex_normals.data();
			mesh.vertexNormalStride = sizeof(float) * 3;
		}
		if (!meshcomponent.vertex_uvset_0.empty()) {
			mesh.vertexUvData = meshcomponent.vertex_uvset_0.data();
			mesh.vertexUvStride = sizeof(float) * 2;
		}
		mesh.indexCount = (int)meshcomponent.indices.size();
		mesh.indexData = meshcomponent.indices.data();
		mesh.indexFormat = xatlas::IndexFormat::UInt32;
		xatlas::AddMeshError::Enum error = xatlas::AddMesh(atlas, mesh);
		if (error != xatlas::AddMeshError::Success) {
			wi::helper::messageBox(xatlas::StringForEnum(error), "Adding mesh to xatlas failed!");
			return dim;
		}
	}

	// Generate atlas:
	{
		xatlas::ChartOptions chartoptions;
		xatlas::ParameterizeOptions parametrizeoptions;
		xatlas::PackOptions packoptions;

		packoptions.resolution = resolution;
		packoptions.blockAlign = true;

		xatlas::Generate(atlas, chartoptions, parametrizeoptions, packoptions);
		dim.width = atlas->width;
		dim.height = atlas->height;

		xatlas::Mesh& mesh = atlas->meshes[0];

		// Note: we must recreate all vertex buffers, because the index buffer will be different (the atlas could have removed shared vertices)
		meshcomponent.indices.clear();
		meshcomponent.indices.resize(mesh.indexCount);
		wi::vector<XMFLOAT3> positions(mesh.vertexCount);
		wi::vector<XMFLOAT2> atlas(mesh.vertexCount);
		wi::vector<XMFLOAT3> normals;
		wi::vector<XMFLOAT4> tangents;
		wi::vector<XMFLOAT2> uvset_0;
		wi::vector<XMFLOAT2> uvset_1;
		wi::vector<uint32_t> colors;
		wi::vector<XMUINT4> boneindices;
		wi::vector<XMFLOAT4> boneweights;
		if (!meshcomponent.vertex_normals.empty())
		{
			normals.resize(mesh.vertexCount);
		}
		if (!meshcomponent.vertex_tangents.empty())
		{
			tangents.resize(mesh.vertexCount);
		}
		if (!meshcomponent.vertex_uvset_0.empty())
		{
			uvset_0.resize(mesh.vertexCount);
		}
		if (!meshcomponent.vertex_uvset_1.empty())
		{
			uvset_1.resize(mesh.vertexCount);
		}
		if (!meshcomponent.vertex_colors.empty())
		{
			colors.resize(mesh.vertexCount);
		}
		if (!meshcomponent.vertex_boneindices.empty())
		{
			boneindices.resize(mesh.vertexCount);
		}
		if (!meshcomponent.vertex_boneweights.empty())
		{
			boneweights.resize(mesh.vertexCount);
		}

		for (uint32_t j = 0; j < mesh.indexCount; ++j)
		{
			const uint32_t ind = mesh.indexArray[j];
			const xatlas::Vertex &v = mesh.vertexArray[ind];
			meshcomponent.indices[j] = ind;
			atlas[ind].x = v.uv[0] / float(dim.width);
			atlas[ind].y = v.uv[1] / float(dim.height);
			positions[ind] = meshcomponent.vertex_positions[v.xref];
			if (!normals.empty())
			{
				normals[ind] = meshcomponent.vertex_normals[v.xref];
			}
			if (!tangents.empty())
			{
				tangents[ind] = meshcomponent.vertex_tangents[v.xref];
			}
			if (!uvset_0.empty())
			{
				uvset_0[ind] = meshcomponent.vertex_uvset_0[v.xref];
			}
			if (!uvset_1.empty())
			{
				uvset_1[ind] = meshcomponent.vertex_uvset_1[v.xref];
			}
			if (!colors.empty())
			{
				colors[ind] = meshcomponent.vertex_colors[v.xref];
			}
			if (!boneindices.empty())
			{
				boneindices[ind] = meshcomponent.vertex_boneindices[v.xref];
			}
			if (!boneweights.empty())
			{
				boneweights[ind] = meshcomponent.vertex_boneweights[v.xref];
			}
		}

		meshcomponent.vertex_positions = positions;
		meshcomponent.vertex_atlas = atlas;
		if (!normals.empty())
		{
			meshcomponent.vertex_normals = normals;
		}
		if (!tangents.empty())
		{
			meshcomponent.vertex_tangents = tangents;
		}
		if (!uvset_0.empty())
		{
			meshcomponent.vertex_uvset_0 = uvset_0;
		}
		if (!uvset_1.empty())
		{
			meshcomponent.vertex_uvset_1 = uvset_1;
		}
		if (!colors.empty())
		{
			meshcomponent.vertex_colors = colors;
		}
		if (!boneindices.empty())
		{
			meshcomponent.vertex_boneindices = boneindices;
		}
		if (!boneweights.empty())
		{
			meshcomponent.vertex_boneweights = boneweights;
		}
		meshcomponent.CreateRenderData();

	}

	//// DEBUG
	//{
	//	const uint32_t width = objectcomponent.lightmapWidth;
	//	const uint32_t height = objectcomponent.lightmapHeight;
	//	objectcomponent.lightmapTextureData.resize(width * height * 4);
	//	const xatlas::OutputMesh *mesh = xatlas::GetOutputMeshes(atlas)[0];
	//	// Rasterize mesh triangles.
	//	const uint8_t white[] = { 255, 255, 255 };
	//	for (uint32_t j = 0; j < mesh->indexCount; j += 3) {
	//		int verts[3][2];
	//		uint8_t color[4];
	//		for (int k = 0; k < 3; k++) {
	//			const xatlas::OutputVertex &v = mesh->vertexArray[mesh->indexArray[j + k]];
	//			verts[k][0] = int(v.uv[0]);
	//			verts[k][1] = int(v.uv[1]);
	//			color[k] = rand() % 255;
	//		}
	//		color[3] = 255;
	//		if (!verts[0][0] && !verts[0][1] && !verts[1][0] && !verts[1][1] && !verts[2][0] && !verts[2][1])
	//			continue; // Skip triangles that weren't atlased.
	//		RasterizeTriangle(objectcomponent.lightmapTextureData.data(), width, verts[0], verts[1], verts[2], color);
	//		RasterizeLine(objectcomponent.lightmapTextureData.data(), width, verts[0], verts[1], white);
	//		RasterizeLine(objectcomponent.lightmapTextureData.data(), width, verts[1], verts[2], white);
	//		RasterizeLine(objectcomponent.lightmapTextureData.data(), width, verts[2], verts[0], white);
	//	}
	//}

	xatlas::Destroy(atlas);

	return dim;
}


void ObjectWindow::Create(EditorComponent* editor)
{
	this->editor = editor;

	wi::gui::Window::Create("Object Window");
	SetSize(XMFLOAT2(660, 500));

	float x = 200;
	float y = 0;
	float hei = 18;
	float step = hei + 2;

	nameLabel.Create("NAMELABEL");
	nameLabel.SetText("");
	nameLabel.SetPos(XMFLOAT2(x - 30, y += step));
	nameLabel.SetSize(XMFLOAT2(150, hei));
	AddWidget(&nameLabel);

	renderableCheckBox.Create("Renderable: ");
	renderableCheckBox.SetTooltip("Set object to be participating in rendering.");
	renderableCheckBox.SetSize(XMFLOAT2(hei, hei));
	renderableCheckBox.SetPos(XMFLOAT2(x, y += step));
	renderableCheckBox.SetCheck(true);
	renderableCheckBox.OnClick([&](wi::gui::EventArgs args) {
		ObjectComponent* object = wi::scene::GetScene().objects.GetComponent(entity);
		if (object != nullptr)
		{
			object->SetRenderable(args.bValue);
		}
	});
	AddWidget(&renderableCheckBox);

	shadowCheckBox.Create("Cast Shadow: ");
	shadowCheckBox.SetTooltip("Set object to be participating in shadows.");
	shadowCheckBox.SetSize(XMFLOAT2(hei, hei));
	shadowCheckBox.SetPos(XMFLOAT2(x, y += step));
	shadowCheckBox.SetCheck(true);
	shadowCheckBox.OnClick([&](wi::gui::EventArgs args) {
		ObjectComponent* object = wi::scene::GetScene().objects.GetComponent(entity);
		if (object != nullptr)
		{
			object->SetCastShadow(args.bValue);
		}
		});
	AddWidget(&shadowCheckBox);

	ditherSlider.Create(0, 1, 0, 1000, "Transparency: ");
	ditherSlider.SetTooltip("Adjust transparency of the object. Opaque materials will use dithered transparency in this case!");
	ditherSlider.SetSize(XMFLOAT2(100, hei));
	ditherSlider.SetPos(XMFLOAT2(x, y += step));
	ditherSlider.OnSlide([&](wi::gui::EventArgs args) {
		ObjectComponent* object = wi::scene::GetScene().objects.GetComponent(entity);
		if (object != nullptr)
		{
			object->color.w = 1 - args.fValue;
		}
	});
	AddWidget(&ditherSlider);

	cascadeMaskSlider.Create(0, 3, 0, 3, "Cascade Mask: ");
	cascadeMaskSlider.SetTooltip("How many shadow cascades to skip when rendering this object into shadow maps? (0: skip none, it will be in all cascades, 1: skip first (biggest cascade), ...etc...");
	cascadeMaskSlider.SetSize(XMFLOAT2(100, hei));
	cascadeMaskSlider.SetPos(XMFLOAT2(x, y += step));
	cascadeMaskSlider.OnSlide([&](wi::gui::EventArgs args) {
		ObjectComponent* object = wi::scene::GetScene().objects.GetComponent(entity);
		if (object != nullptr)
		{
			object->cascadeMask = (uint32_t)args.iValue;
		}
	});
	AddWidget(&cascadeMaskSlider);

	y += step;

	physicsLabel.Create("PHYSICSLABEL");
	physicsLabel.SetText("PHYSICS SETTINGS");
	physicsLabel.SetPos(XMFLOAT2(x - 30, y += step));
	physicsLabel.SetSize(XMFLOAT2(150, hei));
	AddWidget(&physicsLabel);


	collisionShapeComboBox.Create("Collision Shape: ");
	collisionShapeComboBox.SetSize(XMFLOAT2(100, hei));
	collisionShapeComboBox.SetPos(XMFLOAT2(x, y += step));
	collisionShapeComboBox.AddItem("DISABLED");
	collisionShapeComboBox.AddItem("Box");
	collisionShapeComboBox.AddItem("Sphere");
	collisionShapeComboBox.AddItem("Capsule");
	collisionShapeComboBox.AddItem("Convex Hull");
	collisionShapeComboBox.AddItem("Triangle Mesh");
	collisionShapeComboBox.OnSelect([&](wi::gui::EventArgs args)
		{
			if (entity == INVALID_ENTITY)
				return;

			Scene& scene = wi::scene::GetScene();
			RigidBodyPhysicsComponent* physicscomponent = scene.rigidbodies.GetComponent(entity);

			if (args.iValue == 0)
			{
				if (physicscomponent != nullptr)
				{
					scene.rigidbodies.Remove(entity);
				}
				return;
			}

			if (physicscomponent == nullptr)
			{
				physicscomponent = &scene.rigidbodies.Create(entity);
				physicscomponent->SetKinematic(kinematicCheckBox.GetCheck());
				physicscomponent->SetDisableDeactivation(disabledeactivationCheckBox.GetCheck());
				physicscomponent->shape = (RigidBodyPhysicsComponent::CollisionShape)collisionShapeComboBox.GetSelected();
			}

			if (physicscomponent != nullptr)
			{
				XSlider.SetEnabled(false);
				YSlider.SetEnabled(false);
				ZSlider.SetEnabled(false);
				XSlider.SetText("-");
				YSlider.SetText("-");
				ZSlider.SetText("-");

				switch (args.iValue)
				{
				case 1:
					if (physicscomponent->shape != RigidBodyPhysicsComponent::CollisionShape::BOX)
					{
						physicscomponent->physicsobject = nullptr;
						physicscomponent->shape = RigidBodyPhysicsComponent::CollisionShape::BOX;
					}
					XSlider.SetEnabled(true);
					YSlider.SetEnabled(true);
					ZSlider.SetEnabled(true);
					XSlider.SetText("Width");
					YSlider.SetText("Height");
					ZSlider.SetText("Depth");
					XSlider.SetValue(physicscomponent->box.halfextents.x);
					YSlider.SetValue(physicscomponent->box.halfextents.y);
					ZSlider.SetValue(physicscomponent->box.halfextents.z);
					break;
				case 2:
					if (physicscomponent->shape != RigidBodyPhysicsComponent::CollisionShape::SPHERE)
					{
						physicscomponent->physicsobject = nullptr;
						physicscomponent->shape = RigidBodyPhysicsComponent::CollisionShape::SPHERE;
					}
					XSlider.SetEnabled(true);
					XSlider.SetText("Radius");
					YSlider.SetText("-");
					ZSlider.SetText("-");
					XSlider.SetValue(physicscomponent->sphere.radius);
					break;
				case 3:
					if (physicscomponent->shape != RigidBodyPhysicsComponent::CollisionShape::CAPSULE)
					{
						physicscomponent->physicsobject = nullptr;
						physicscomponent->shape = RigidBodyPhysicsComponent::CollisionShape::CAPSULE;
					}
					XSlider.SetEnabled(true);
					YSlider.SetEnabled(true);
					XSlider.SetText("Height");
					YSlider.SetText("Radius");
					ZSlider.SetText("-");
					XSlider.SetValue(physicscomponent->capsule.height);
					YSlider.SetValue(physicscomponent->capsule.radius);
					break;
				case 4:
					if (physicscomponent->shape != RigidBodyPhysicsComponent::CollisionShape::CONVEX_HULL)
					{
						physicscomponent->physicsobject = nullptr;
						physicscomponent->shape = RigidBodyPhysicsComponent::CollisionShape::CONVEX_HULL;
					}
					break;
				case 5:
					if (physicscomponent->shape != RigidBodyPhysicsComponent::CollisionShape::TRIANGLE_MESH)
					{
						physicscomponent->physicsobject = nullptr;
						physicscomponent->shape = RigidBodyPhysicsComponent::CollisionShape::TRIANGLE_MESH;
					}
					break;
				default:
					break;
				}
			}
		});
	collisionShapeComboBox.SetSelected(0);
	collisionShapeComboBox.SetEnabled(true);
	collisionShapeComboBox.SetTooltip("Set rigid body collision shape.");
	AddWidget(&collisionShapeComboBox);

	XSlider.Create(0, 10, 1, 100000, "X: ");
	XSlider.SetSize(XMFLOAT2(100, hei));
	XSlider.SetPos(XMFLOAT2(x, y += step));
	XSlider.OnSlide([&](wi::gui::EventArgs args) {
		RigidBodyPhysicsComponent* physicscomponent = wi::scene::GetScene().rigidbodies.GetComponent(entity);
		if (physicscomponent != nullptr)
		{
			switch (physicscomponent->shape)
			{
			default:
			case RigidBodyPhysicsComponent::CollisionShape::BOX:
				physicscomponent->box.halfextents.x = args.fValue;
				break;
			case RigidBodyPhysicsComponent::CollisionShape::SPHERE:
				physicscomponent->sphere.radius = args.fValue;
				break;
			case RigidBodyPhysicsComponent::CollisionShape::CAPSULE:
				physicscomponent->capsule.height = args.fValue;
				break;
			}
			physicscomponent->physicsobject = nullptr;
		}
		});
	AddWidget(&XSlider);

	YSlider.Create(0, 10, 1, 100000, "Y: ");
	YSlider.SetSize(XMFLOAT2(100, hei));
	YSlider.SetPos(XMFLOAT2(x, y += step));
	YSlider.OnSlide([&](wi::gui::EventArgs args) {
		RigidBodyPhysicsComponent* physicscomponent = wi::scene::GetScene().rigidbodies.GetComponent(entity);
		if (physicscomponent != nullptr)
		{
			switch (physicscomponent->shape)
			{
			default:
			case RigidBodyPhysicsComponent::CollisionShape::BOX:
				physicscomponent->box.halfextents.y = args.fValue;
				break;
			case RigidBodyPhysicsComponent::CollisionShape::CAPSULE:
				physicscomponent->capsule.radius = args.fValue;
				break;
			}
			physicscomponent->physicsobject = nullptr;
		}
		});
	AddWidget(&YSlider);

	ZSlider.Create(0, 10, 1, 100000, "Z: ");
	ZSlider.SetSize(XMFLOAT2(100, hei));
	ZSlider.SetPos(XMFLOAT2(x, y += step));
	ZSlider.OnSlide([&](wi::gui::EventArgs args) {
		RigidBodyPhysicsComponent* physicscomponent = wi::scene::GetScene().rigidbodies.GetComponent(entity);
		if (physicscomponent != nullptr)
		{
			switch (physicscomponent->shape)
			{
			default:
			case RigidBodyPhysicsComponent::CollisionShape::BOX:
				physicscomponent->box.halfextents.z = args.fValue;
				break;
			}
			physicscomponent->physicsobject = nullptr;
		}
		});
	AddWidget(&ZSlider);

	massSlider.Create(0, 10, 1, 100000, "Mass: ");
	massSlider.SetTooltip("Set the mass amount for the physics engine.");
	massSlider.SetSize(XMFLOAT2(100, hei));
	massSlider.SetPos(XMFLOAT2(x, y += step));
	massSlider.OnSlide([&](wi::gui::EventArgs args) {
		RigidBodyPhysicsComponent* physicscomponent = wi::scene::GetScene().rigidbodies.GetComponent(entity);
		if (physicscomponent != nullptr)
		{
			physicscomponent->mass = args.fValue;
		}
		});
	AddWidget(&massSlider);

	frictionSlider.Create(0, 1, 0.5f, 100000, "Friction: ");
	frictionSlider.SetTooltip("Set the friction amount for the physics engine.");
	frictionSlider.SetSize(XMFLOAT2(100, hei));
	frictionSlider.SetPos(XMFLOAT2(x, y += step));
	frictionSlider.OnSlide([&](wi::gui::EventArgs args) {
		RigidBodyPhysicsComponent* physicscomponent = wi::scene::GetScene().rigidbodies.GetComponent(entity);
		if (physicscomponent != nullptr)
		{
			physicscomponent->friction = args.fValue;
		}
		});
	AddWidget(&frictionSlider);

	restitutionSlider.Create(0, 1, 0, 100000, "Restitution: ");
	restitutionSlider.SetTooltip("Set the restitution amount for the physics engine.");
	restitutionSlider.SetSize(XMFLOAT2(100, hei));
	restitutionSlider.SetPos(XMFLOAT2(x, y += step));
	restitutionSlider.OnSlide([&](wi::gui::EventArgs args) {
		RigidBodyPhysicsComponent* physicscomponent = wi::scene::GetScene().rigidbodies.GetComponent(entity);
		if (physicscomponent != nullptr)
		{
			physicscomponent->restitution = args.fValue;
		}
		});
	AddWidget(&restitutionSlider);

	lineardampingSlider.Create(0, 1, 0, 100000, "Linear Damping: ");
	lineardampingSlider.SetTooltip("Set the linear damping amount for the physics engine.");
	lineardampingSlider.SetSize(XMFLOAT2(100, hei));
	lineardampingSlider.SetPos(XMFLOAT2(x, y += step));
	lineardampingSlider.OnSlide([&](wi::gui::EventArgs args) {
		RigidBodyPhysicsComponent* physicscomponent = wi::scene::GetScene().rigidbodies.GetComponent(entity);
		if (physicscomponent != nullptr)
		{
			physicscomponent->damping_linear = args.fValue;
		}
		});
	AddWidget(&lineardampingSlider);

	angulardampingSlider.Create(0, 1, 0, 100000, "Angular Damping: ");
	angulardampingSlider.SetTooltip("Set the mass amount for the physics engine.");
	angulardampingSlider.SetSize(XMFLOAT2(100, hei));
	angulardampingSlider.SetPos(XMFLOAT2(x, y += step));
	angulardampingSlider.OnSlide([&](wi::gui::EventArgs args) {
		RigidBodyPhysicsComponent* physicscomponent = wi::scene::GetScene().rigidbodies.GetComponent(entity);
		if (physicscomponent != nullptr)
		{
			physicscomponent->damping_angular = args.fValue;
		}
		});
	AddWidget(&angulardampingSlider);

	kinematicCheckBox.Create("Kinematic: ");
	kinematicCheckBox.SetTooltip("Toggle kinematic behaviour.");
	kinematicCheckBox.SetSize(XMFLOAT2(hei, hei));
	kinematicCheckBox.SetPos(XMFLOAT2(x, y += step));
	kinematicCheckBox.SetCheck(false);
	kinematicCheckBox.OnClick([&](wi::gui::EventArgs args) {
		RigidBodyPhysicsComponent* physicscomponent = wi::scene::GetScene().rigidbodies.GetComponent(entity);
		if (physicscomponent != nullptr)
		{
			physicscomponent->SetKinematic(args.bValue);
		}
	});
	AddWidget(&kinematicCheckBox);

	disabledeactivationCheckBox.Create("Disable Deactivation: ");
	disabledeactivationCheckBox.SetTooltip("Toggle kinematic behaviour.");
	disabledeactivationCheckBox.SetSize(XMFLOAT2(hei, hei));
	disabledeactivationCheckBox.SetPos(XMFLOAT2(x, y += step));
	disabledeactivationCheckBox.SetCheck(false);
	disabledeactivationCheckBox.OnClick([&](wi::gui::EventArgs args) {
		RigidBodyPhysicsComponent* physicscomponent = wi::scene::GetScene().rigidbodies.GetComponent(entity);
		if (physicscomponent != nullptr)
		{
			physicscomponent->SetDisableDeactivation(args.bValue);
		}
	});
	AddWidget(&disabledeactivationCheckBox);


	y += step;


	lightmapResolutionSlider.Create(32, 1024, 128, 1024 - 32, "Lightmap resolution: ");
	lightmapResolutionSlider.SetTooltip("Set the approximate resolution for this object's lightmap. This will be packed into the larger global lightmap later.");
	lightmapResolutionSlider.SetSize(XMFLOAT2(100, hei));
	lightmapResolutionSlider.SetPos(XMFLOAT2(x, y += step));
	lightmapResolutionSlider.OnSlide([&](wi::gui::EventArgs args) {
		// unfortunately, we must be pow2 with full float lightmap format, otherwise it could be unlimited (but accumulation blending would suffer then)
		//	or at least for me, downloading the lightmap was glitching out when non-pow 2 and RGBA32_FLOAT format
		lightmapResolutionSlider.SetValue(float(wi::math::GetNextPowerOfTwo(uint32_t(args.fValue)))); 
	});
	AddWidget(&lightmapResolutionSlider);

	lightmapSourceUVSetComboBox.Create("UV Set: ");
	lightmapSourceUVSetComboBox.SetPos(XMFLOAT2(x - 130, y += step));
	lightmapSourceUVSetComboBox.AddItem("Copy UV 0");
	lightmapSourceUVSetComboBox.AddItem("Copy UV 1");
	lightmapSourceUVSetComboBox.AddItem("Keep Atlas");
	lightmapSourceUVSetComboBox.AddItem("Generate Atlas");
	lightmapSourceUVSetComboBox.SetSelected(3);
	lightmapSourceUVSetComboBox.SetTooltip("Set which UV set to use when generating the lightmap Atlas");
	AddWidget(&lightmapSourceUVSetComboBox);

	generateLightmapButton.Create("Generate Lightmap");
	generateLightmapButton.SetTooltip("Render the lightmap for only this object. It will automatically combined with the global lightmap.");
	generateLightmapButton.SetPos(XMFLOAT2(x, y));
	generateLightmapButton.SetSize(XMFLOAT2(140, hei));
	generateLightmapButton.OnClick([&](wi::gui::EventArgs args) {

		Scene& scene = wi::scene::GetScene();

		enum UV_GEN_TYPE
		{
			UV_GEN_COPY_UVSET_0,
			UV_GEN_COPY_UVSET_1,
			UV_GEN_KEEP_ATLAS,
			UV_GEN_GENERATE_ATLAS,
		};
		UV_GEN_TYPE gen_type = (UV_GEN_TYPE)lightmapSourceUVSetComboBox.GetSelected();

		wi::unordered_set<ObjectComponent*> gen_objects;
		wi::unordered_map<MeshComponent*, Atlas_Dim> gen_meshes;

		for (auto& x : this->editor->translator.selected)
		{
			ObjectComponent* objectcomponent = scene.objects.GetComponent(x.entity);
			if (objectcomponent != nullptr)
			{
				MeshComponent* meshcomponent = scene.meshes.GetComponent(objectcomponent->meshID);

				if (meshcomponent != nullptr)
				{
					gen_objects.insert(objectcomponent);
					gen_meshes[meshcomponent] = Atlas_Dim();
				}
			}

		}

		wi::jobsystem::context ctx;

		for (auto& it : gen_meshes)
		{
			MeshComponent& mesh = *it.first;
			if (gen_type == UV_GEN_COPY_UVSET_0)
			{
				mesh.vertex_atlas = mesh.vertex_uvset_0;
				mesh.CreateRenderData();
			}
			else if (gen_type == UV_GEN_COPY_UVSET_1)
			{
				mesh.vertex_atlas = mesh.vertex_uvset_1;
				mesh.CreateRenderData();
			}
			else if (gen_type == UV_GEN_GENERATE_ATLAS)
			{
				wi::jobsystem::Execute(ctx, [&](wi::jobsystem::JobArgs args) {
					it.second = GenerateMeshAtlas(mesh, (uint32_t)lightmapResolutionSlider.GetValue());
				});
			}
		}
		wi::jobsystem::Wait(ctx);

		for (auto& x : gen_objects)
		{
			x->ClearLightmap();
			MeshComponent* meshcomponent = scene.meshes.GetComponent(x->meshID);
			if (gen_type == UV_GEN_GENERATE_ATLAS)
			{
				x->lightmapWidth = gen_meshes.at(meshcomponent).width;
				x->lightmapHeight = gen_meshes.at(meshcomponent).height;
			}
			else
			{
				x->lightmapWidth = x->lightmapHeight = (uint32_t)lightmapResolutionSlider.GetValue();
			}
			x->SetLightmapRenderRequest(true);
		}

		scene.SetAccelerationStructureUpdateRequested(true);

	});
	AddWidget(&generateLightmapButton);

	stopLightmapGenButton.Create("Stop Lightmap Gen");
	stopLightmapGenButton.SetTooltip("Stop the lightmap rendering and save the lightmap.");
	stopLightmapGenButton.SetPos(XMFLOAT2(x, y += step));
	stopLightmapGenButton.SetSize(XMFLOAT2(140, hei));
	stopLightmapGenButton.OnClick([&](wi::gui::EventArgs args) {

		Scene& scene = wi::scene::GetScene();

		for (auto& x : this->editor->translator.selected)
		{
			ObjectComponent* objectcomponent = scene.objects.GetComponent(x.entity);
			if (objectcomponent != nullptr)
			{
				objectcomponent->SetLightmapRenderRequest(false);
				objectcomponent->SaveLightmap();
			}
		}

	});
	AddWidget(&stopLightmapGenButton);

	clearLightmapButton.Create("Clear Lightmap");
	clearLightmapButton.SetTooltip("Clear the lightmap from this object.");
	clearLightmapButton.SetPos(XMFLOAT2(x, y += step));
	clearLightmapButton.SetSize(XMFLOAT2(140, hei));
	clearLightmapButton.OnClick([&](wi::gui::EventArgs args) {

		Scene& scene = wi::scene::GetScene();

		for (auto& x : this->editor->translator.selected)
		{
			ObjectComponent* objectcomponent = scene.objects.GetComponent(x.entity);
			if (objectcomponent != nullptr)
			{
				objectcomponent->ClearLightmap();
			}
		}

	});
	AddWidget(&clearLightmapButton);

	y = 10;

	colorComboBox.Create("Color picker mode: ");
	colorComboBox.SetSize(XMFLOAT2(120, hei));
	colorComboBox.SetPos(XMFLOAT2(x + 300, y += step));
	colorComboBox.AddItem("Base color");
	colorComboBox.AddItem("Emissive color");
	colorComboBox.SetTooltip("Choose the destination data of the color picker.");
	AddWidget(&colorComboBox);

	colorPicker.Create("Object Color", false);
	colorPicker.SetPos(XMFLOAT2(350, y += step));
	colorPicker.SetVisible(true);
	colorPicker.SetEnabled(true);
	colorPicker.OnColorChanged([&](wi::gui::EventArgs args) {
		ObjectComponent* object = wi::scene::GetScene().objects.GetComponent(entity);
		if (object != nullptr)
		{
			switch (colorComboBox.GetSelected())
			{
			default:
			case 0:
			{
				XMFLOAT3 col = args.color.toFloat3();
				object->color = XMFLOAT4(col.x, col.y, col.z, object->color.w);
			}
			break;
			case 1:
				object->emissiveColor = args.color.toFloat4();
				break;
			}
		}
		});
	AddWidget(&colorPicker);


	Translate(XMFLOAT3((float)editor->GetLogicalWidth() - 720, 120, 0));
	SetVisible(false);

	SetEntity(INVALID_ENTITY);
}


void ObjectWindow::SetEntity(Entity entity)
{
	if (this->entity == entity)
		return;

	this->entity = entity;

	Scene& scene = wi::scene::GetScene();

	const ObjectComponent* object = scene.objects.GetComponent(entity);

	if (object != nullptr)
	{
		SetEnabled(true);

		const NameComponent* name = scene.names.GetComponent(entity);
		nameLabel.SetText(name == nullptr ? std::to_string(entity) : name->name);

		renderableCheckBox.SetCheck(object->IsRenderable());
		shadowCheckBox.SetCheck(object->IsCastingShadow());
		cascadeMaskSlider.SetValue((float)object->cascadeMask);
		ditherSlider.SetValue(object->GetTransparency());

		switch (colorComboBox.GetSelected())
		{
		default:
		case 0:
			colorPicker.SetPickColor(wi::Color::fromFloat4(object->color));
			break;
		case 1:
			colorPicker.SetPickColor(wi::Color::fromFloat4(object->emissiveColor));
			break;
		}

	}
	else
	{
		SetEnabled(false);
	}

	const TransformComponent* transform = scene.transforms.GetComponent(entity);
	const RigidBodyPhysicsComponent* physicsComponent = scene.rigidbodies.GetComponent(entity);

	if (transform != nullptr)
	{
		kinematicCheckBox.SetEnabled(true);
		disabledeactivationCheckBox.SetEnabled(true);
		collisionShapeComboBox.SetEnabled(true);
		massSlider.SetEnabled(true);
		frictionSlider.SetEnabled(true);
		restitutionSlider.SetEnabled(true);
		lineardampingSlider.SetEnabled(true);
		angulardampingSlider.SetEnabled(true);

		if (physicsComponent != nullptr)
		{
			massSlider.SetValue(physicsComponent->mass);
			frictionSlider.SetValue(physicsComponent->friction);
			restitutionSlider.SetValue(physicsComponent->restitution);
			lineardampingSlider.SetValue(physicsComponent->damping_linear);
			angulardampingSlider.SetValue(physicsComponent->damping_angular);

			kinematicCheckBox.SetCheck(physicsComponent->IsKinematic());
			disabledeactivationCheckBox.SetCheck(physicsComponent->IsDisableDeactivation());

			if (physicsComponent->shape == RigidBodyPhysicsComponent::CollisionShape::BOX)
			{
				collisionShapeComboBox.SetSelected(1);
			}
			else if (physicsComponent->shape == RigidBodyPhysicsComponent::CollisionShape::SPHERE)
			{
				collisionShapeComboBox.SetSelected(2);
			}
			else if (physicsComponent->shape == RigidBodyPhysicsComponent::CollisionShape::CAPSULE)
			{
				collisionShapeComboBox.SetSelected(3);
			}
			else if (physicsComponent->shape == RigidBodyPhysicsComponent::CollisionShape::CONVEX_HULL)
			{
				collisionShapeComboBox.SetSelected(4);
			}
			else if (physicsComponent->shape == RigidBodyPhysicsComponent::CollisionShape::TRIANGLE_MESH)
			{
				collisionShapeComboBox.SetSelected(5);
			}
		}
		else
		{
			collisionShapeComboBox.SetSelected(0);
			kinematicCheckBox.SetCheck(false);
			disabledeactivationCheckBox.SetCheck(false);
		}
	}

}
