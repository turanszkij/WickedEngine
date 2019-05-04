#include "stdafx.h"
#include "Editor.h"
#include "ObjectWindow.h"
#include "wiSceneSystem.h"

#include "xatlas.h"

#include <sstream>
#include <unordered_set>
#include <unordered_map>

using namespace wiECS;
using namespace wiSceneSystem;


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
		xatlas::InputMesh mesh;
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
			wiHelper::messageBox(xatlas::StringForEnum(error), "Adding mesh to xatlas failed!");
			return dim;
		}
	}

	// Generate atlas:
	{
		xatlas::PackerOptions packerOptions;
		packerOptions.resolution = resolution;
		packerOptions.conservative = true;
		packerOptions.padding = 1;
		xatlas::GenerateCharts(atlas, xatlas::CharterOptions(), nullptr, nullptr);
		xatlas::PackCharts(atlas, packerOptions, nullptr, nullptr);
		const uint32_t charts = xatlas::GetNumCharts(atlas);
		dim.width = xatlas::GetWidth(atlas);
		dim.height = xatlas::GetHeight(atlas);
		const xatlas::OutputMesh* mesh = xatlas::GetOutputMeshes(atlas)[0];

		// Note: we must recreate all vertex buffers, because the index buffer will be different (the atlas could have removed shared vertices)
		meshcomponent.indices.clear();
		meshcomponent.indices.resize(mesh->indexCount);
		std::vector<XMFLOAT3> positions(mesh->vertexCount);
		std::vector<XMFLOAT2> atlas(mesh->vertexCount);
		std::vector<XMFLOAT3> normals;
		std::vector<XMFLOAT2> uvset_0;
		std::vector<XMFLOAT2> uvset_1;
		std::vector<uint32_t> colors;
		std::vector<XMUINT4> boneindices;
		std::vector<XMFLOAT4> boneweights;
		if (!meshcomponent.vertex_normals.empty())
		{
			normals.resize(mesh->vertexCount);
		}
		if (!meshcomponent.vertex_uvset_0.empty())
		{
			uvset_0.resize(mesh->vertexCount);
		}
		if (!meshcomponent.vertex_uvset_1.empty())
		{
			uvset_1.resize(mesh->vertexCount);
		}
		if (!meshcomponent.vertex_colors.empty())
		{
			colors.resize(mesh->vertexCount);
		}
		if (!meshcomponent.vertex_boneindices.empty())
		{
			boneindices.resize(mesh->vertexCount);
		}
		if (!meshcomponent.vertex_boneweights.empty())
		{
			boneweights.resize(mesh->vertexCount);
		}

		for (uint32_t j = 0; j < mesh->indexCount; ++j)
		{
			const uint32_t ind = mesh->indexArray[j];
			const xatlas::OutputVertex &v = mesh->vertexArray[ind];
			meshcomponent.indices[j] = ind;
			atlas[ind].x = v.uv[0] / float(dim.width);
			atlas[ind].y = v.uv[1] / float(dim.height);
			positions[ind] = meshcomponent.vertex_positions[v.xref];
			if (!normals.empty())
			{
				normals[ind] = meshcomponent.vertex_normals[v.xref];
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


ObjectWindow::ObjectWindow(EditorComponent* editor) : editor(editor)
{
	GUI = &editor->GetGUI();
	assert(GUI && "Invalid GUI!");


	float screenW = (float)wiRenderer::GetDevice()->GetScreenWidth();
	float screenH = (float)wiRenderer::GetDevice()->GetScreenHeight();

	objectWindow = new wiWindow(GUI, "Object Window");
	objectWindow->SetSize(XMFLOAT2(600, 520));
	objectWindow->SetEnabled(false);
	GUI->AddWidget(objectWindow);

	float x = 450;
	float y = 0;

	nameLabel = new wiLabel("NAMELABEL");
	nameLabel->SetText("");
	nameLabel->SetPos(XMFLOAT2(x - 30, y += 30));
	nameLabel->SetSize(XMFLOAT2(150, 20));
	objectWindow->AddWidget(nameLabel);

	renderableCheckBox = new wiCheckBox("Renderable: ");
	renderableCheckBox->SetTooltip("Set object to be participating in rendering.");
	renderableCheckBox->SetPos(XMFLOAT2(x, y += 30));
	renderableCheckBox->SetCheck(true);
	renderableCheckBox->OnClick([&](wiEventArgs args) {
		ObjectComponent* object = wiSceneSystem::GetScene().objects.GetComponent(entity);
		if (object != nullptr)
		{
			object->SetRenderable(args.bValue);
		}
	});
	objectWindow->AddWidget(renderableCheckBox);

	ditherSlider = new wiSlider(0, 1, 0, 1000, "Dither: ");
	ditherSlider->SetTooltip("Adjust dithered transparency of the object. This disables some optimizations so performance can be affected.");
	ditherSlider->SetSize(XMFLOAT2(100, 30));
	ditherSlider->SetPos(XMFLOAT2(x, y += 30));
	ditherSlider->OnSlide([&](wiEventArgs args) {
		ObjectComponent* object = wiSceneSystem::GetScene().objects.GetComponent(entity);
		if (object != nullptr)
		{
			object->color.w = 1 - args.fValue;
		}
	});
	objectWindow->AddWidget(ditherSlider);

	cascadeMaskSlider = new wiSlider(0, 3, 0, 3, "Cascade Mask: ");
	cascadeMaskSlider->SetTooltip("How many shadow cascades to skip when rendering this object into shadow maps? (0: skip none, it will be in all cascades, 1: skip first (biggest cascade), ...etc...");
	cascadeMaskSlider->SetSize(XMFLOAT2(100, 30));
	cascadeMaskSlider->SetPos(XMFLOAT2(x, y += 30));
	cascadeMaskSlider->OnSlide([&](wiEventArgs args) {
		ObjectComponent* object = wiSceneSystem::GetScene().objects.GetComponent(entity);
		if (object != nullptr)
		{
			object->cascadeMask = (uint32_t)args.iValue;
		}
	});
	objectWindow->AddWidget(cascadeMaskSlider);


	colorPicker = new wiColorPicker(GUI, "Object Color");
	colorPicker->SetPos(XMFLOAT2(10, 30));
	colorPicker->RemoveWidgets();
	colorPicker->SetVisible(true);
	colorPicker->SetEnabled(true);
	colorPicker->OnColorChanged([&](wiEventArgs args) {
		ObjectComponent* object = wiSceneSystem::GetScene().objects.GetComponent(entity);
		if (object != nullptr)
		{
			XMFLOAT3 col = args.color.toFloat3();
			object->color = XMFLOAT4(col.x, col.y, col.z, object->color.w);
		}
	});
	objectWindow->AddWidget(colorPicker);

	y += 60;

	physicsLabel = new wiLabel("PHYSICSLABEL");
	physicsLabel->SetText("PHYSICS SETTINGS");
	physicsLabel->SetPos(XMFLOAT2(x - 30, y += 30));
	physicsLabel->SetSize(XMFLOAT2(150, 20));
	objectWindow->AddWidget(physicsLabel);



	rigidBodyCheckBox = new wiCheckBox("Rigid Body Physics: ");
	rigidBodyCheckBox->SetTooltip("Enable rigid body physics simulation.");
	rigidBodyCheckBox->SetPos(XMFLOAT2(x, y += 30));
	rigidBodyCheckBox->SetCheck(false);
	rigidBodyCheckBox->OnClick([&](wiEventArgs args) 
	{
		Scene& scene = wiSceneSystem::GetScene();
		RigidBodyPhysicsComponent* physicscomponent = scene.rigidbodies.GetComponent(entity);

		if (args.bValue)
		{
			if (physicscomponent == nullptr)
			{
				RigidBodyPhysicsComponent& rigidbody = scene.rigidbodies.Create(entity);
				rigidbody.SetKinematic(kinematicCheckBox->GetCheck());
				rigidbody.SetDisableDeactivation(disabledeactivationCheckBox->GetCheck());
				rigidbody.shape = (RigidBodyPhysicsComponent::CollisionShape)collisionShapeComboBox->GetSelected();
			}
		}
		else
		{
			if (physicscomponent != nullptr)
			{
				scene.rigidbodies.Remove(entity);
			}
		}

	});
	objectWindow->AddWidget(rigidBodyCheckBox);

	kinematicCheckBox = new wiCheckBox("Kinematic: ");
	kinematicCheckBox->SetTooltip("Toggle kinematic behaviour.");
	kinematicCheckBox->SetPos(XMFLOAT2(x, y += 30));
	kinematicCheckBox->SetCheck(false);
	kinematicCheckBox->OnClick([&](wiEventArgs args) {
		RigidBodyPhysicsComponent* physicscomponent = wiSceneSystem::GetScene().rigidbodies.GetComponent(entity);
		if (physicscomponent != nullptr)
		{
			physicscomponent->SetKinematic(args.bValue);
		}
	});
	objectWindow->AddWidget(kinematicCheckBox);

	disabledeactivationCheckBox = new wiCheckBox("Disable Deactivation: ");
	disabledeactivationCheckBox->SetTooltip("Toggle kinematic behaviour.");
	disabledeactivationCheckBox->SetPos(XMFLOAT2(x, y += 30));
	disabledeactivationCheckBox->SetCheck(false);
	disabledeactivationCheckBox->OnClick([&](wiEventArgs args) {
		RigidBodyPhysicsComponent* physicscomponent = wiSceneSystem::GetScene().rigidbodies.GetComponent(entity);
		if (physicscomponent != nullptr)
		{
			physicscomponent->SetDisableDeactivation(args.bValue);
		}
	});
	objectWindow->AddWidget(disabledeactivationCheckBox);

	collisionShapeComboBox = new wiComboBox("Collision Shape:");
	collisionShapeComboBox->SetSize(XMFLOAT2(100, 20));
	collisionShapeComboBox->SetPos(XMFLOAT2(x, y += 30));
	collisionShapeComboBox->AddItem("Box");
	collisionShapeComboBox->AddItem("Sphere");
	collisionShapeComboBox->AddItem("Capsule");
	collisionShapeComboBox->AddItem("Convex Hull");
	collisionShapeComboBox->AddItem("Triangle Mesh");
	collisionShapeComboBox->OnSelect([&](wiEventArgs args) 
	{
		RigidBodyPhysicsComponent* physicscomponent = wiSceneSystem::GetScene().rigidbodies.GetComponent(entity);
		if (physicscomponent != nullptr)
		{
			switch (args.iValue)
			{
			case 0:
				physicscomponent->shape = RigidBodyPhysicsComponent::CollisionShape::BOX;
				break;
			case 1:
				physicscomponent->shape = RigidBodyPhysicsComponent::CollisionShape::SPHERE;
				break;
			case 2:
				physicscomponent->shape = RigidBodyPhysicsComponent::CollisionShape::CAPSULE;
				break;
			case 3:
				physicscomponent->shape = RigidBodyPhysicsComponent::CollisionShape::CONVEX_HULL;
				break;
			case 4:
				physicscomponent->shape = RigidBodyPhysicsComponent::CollisionShape::TRIANGLE_MESH;
				break;
			default:
				break;
			}
		}
	});
	collisionShapeComboBox->SetSelected(0);
	collisionShapeComboBox->SetEnabled(true);
	collisionShapeComboBox->SetTooltip("Set rigid body collision shape.");
	objectWindow->AddWidget(collisionShapeComboBox);


	y += 30;


	lightmapResolutionSlider = new wiSlider(32, 1024, 128, 1024 - 32, "Lightmap resolution: ");
	lightmapResolutionSlider->SetTooltip("Set the approximate resolution for this object's lightmap. This will be packed into the larger global lightmap later.");
	lightmapResolutionSlider->SetSize(XMFLOAT2(100, 30));
	lightmapResolutionSlider->SetPos(XMFLOAT2(x, y += 30));
	lightmapResolutionSlider->OnSlide([&](wiEventArgs args) {
		// unfortunately, we must be pow2 with full float lightmap format, otherwise it could be unlimited (but accumulation blending would suffer then)
		//	or at least for me, downloading the lightmap was glitching out when non-pow 2 and RGBA32_FLOAT format
		lightmapResolutionSlider->SetValue(float(wiMath::GetNextPowerOfTwo(uint32_t(args.fValue)))); 
	});
	objectWindow->AddWidget(lightmapResolutionSlider);

	lightmapSourceUVSetComboBox = new wiComboBox("Source UV: ");
	lightmapSourceUVSetComboBox->SetPos(XMFLOAT2(x - 130, y += 30));
	lightmapSourceUVSetComboBox->AddItem("Copy UV 0");
	lightmapSourceUVSetComboBox->AddItem("Copy UV 1");
	lightmapSourceUVSetComboBox->AddItem("Keep Atlas");
	lightmapSourceUVSetComboBox->AddItem("Generate Atlas");
	lightmapSourceUVSetComboBox->SetSelected(3);
	lightmapSourceUVSetComboBox->SetTooltip("Set which UV set to use when generating the lightmap Atlas");
	objectWindow->AddWidget(lightmapSourceUVSetComboBox);

	generateLightmapButton = new wiButton("Generate Lightmap");
	generateLightmapButton->SetTooltip("Render the lightmap for only this object. It will automatically combined with the global lightmap.");
	generateLightmapButton->SetPos(XMFLOAT2(x, y));
	generateLightmapButton->SetSize(XMFLOAT2(140,30));
	generateLightmapButton->OnClick([&](wiEventArgs args) {

		Scene& scene = wiSceneSystem::GetScene();

		enum UV_GEN_TYPE
		{
			UV_GEN_COPY_UVSET_0,
			UV_GEN_COPY_UVSET_1,
			UV_GEN_KEEP_ATLAS,
			UV_GEN_GENERATE_ATLAS,
		};
		UV_GEN_TYPE gen_type = (UV_GEN_TYPE)lightmapSourceUVSetComboBox->GetSelected();

		std::unordered_set<ObjectComponent*> gen_objects;
		std::unordered_map<MeshComponent*, Atlas_Dim> gen_meshes;

		for (auto& x : this->editor->selected)
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
				wiJobSystem::Execute([&] {
					it.second = GenerateMeshAtlas(mesh, (uint32_t)lightmapResolutionSlider->GetValue());
				});
			}
		}
		wiJobSystem::Wait();

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
				x->lightmapWidth = x->lightmapHeight = (uint32_t)lightmapResolutionSlider->GetValue();
			}
			x->SetLightmapRenderRequest(true);
		}

		wiRenderer::InvalidateBVH();

	});
	objectWindow->AddWidget(generateLightmapButton);

	stopLightmapGenButton = new wiButton("Stop Lightmap Gen");
	stopLightmapGenButton->SetTooltip("Stop the lightmap rendering and save the lightmap.");
	stopLightmapGenButton->SetPos(XMFLOAT2(x, y += 30));
	stopLightmapGenButton->SetSize(XMFLOAT2(140, 30));
	stopLightmapGenButton->OnClick([&](wiEventArgs args) {

		Scene& scene = wiSceneSystem::GetScene();

		for (auto& x : this->editor->selected)
		{
			ObjectComponent* objectcomponent = scene.objects.GetComponent(x.entity);
			if (objectcomponent != nullptr)
			{
				objectcomponent->SetLightmapRenderRequest(false);
				objectcomponent->SaveLightmap();
			}
		}

	});
	objectWindow->AddWidget(stopLightmapGenButton);

	clearLightmapButton = new wiButton("Clear Lightmap");
	clearLightmapButton->SetTooltip("Clear the lightmap from this object.");
	clearLightmapButton->SetPos(XMFLOAT2(x, y += 30));
	clearLightmapButton->SetSize(XMFLOAT2(140, 30));
	clearLightmapButton->OnClick([&](wiEventArgs args) {

		Scene& scene = wiSceneSystem::GetScene();

		for (auto& x : this->editor->selected)
		{
			ObjectComponent* objectcomponent = scene.objects.GetComponent(x.entity);
			if (objectcomponent != nullptr)
			{
				objectcomponent->ClearLightmap();
			}
		}

	});
	objectWindow->AddWidget(clearLightmapButton);



	objectWindow->Translate(XMFLOAT3(screenW - 720, 120, 0));
	objectWindow->SetVisible(false);

	SetEntity(INVALID_ENTITY);
}


ObjectWindow::~ObjectWindow()
{
	objectWindow->RemoveWidgets(true);
	GUI->RemoveWidget(objectWindow);
	SAFE_DELETE(objectWindow);
}


void ObjectWindow::SetEntity(Entity entity)
{
	if (this->entity == entity)
		return;

	this->entity = entity;

	Scene& scene = wiSceneSystem::GetScene();

	const ObjectComponent* object = scene.objects.GetComponent(entity);

	if (object != nullptr)
	{
		const NameComponent* name = scene.names.GetComponent(entity);
		if (name != nullptr)
		{
			std::stringstream ss("");
			ss << name->name << " (" << entity << ")";
			nameLabel->SetText(ss.str());
		}
		else
		{
			std::stringstream ss("");
			ss<< "(" << entity << ")";
			nameLabel->SetText(ss.str());
		}

		renderableCheckBox->SetCheck(object->IsRenderable());
		cascadeMaskSlider->SetValue((float)object->cascadeMask);
		ditherSlider->SetValue(object->GetTransparency());

		const RigidBodyPhysicsComponent* physicsComponent = scene.rigidbodies.GetComponent(entity);

		rigidBodyCheckBox->SetCheck(physicsComponent != nullptr);

		if (physicsComponent != nullptr)
		{
			kinematicCheckBox->SetCheck(physicsComponent->IsKinematic());
			disabledeactivationCheckBox->SetCheck(physicsComponent->IsDisableDeactivation());

			if (physicsComponent->shape == RigidBodyPhysicsComponent::CollisionShape::BOX)
			{
				collisionShapeComboBox->SetSelected(0);
			}
			else if (physicsComponent->shape == RigidBodyPhysicsComponent::CollisionShape::SPHERE)
			{
				collisionShapeComboBox->SetSelected(1);
			}
			else if (physicsComponent->shape == RigidBodyPhysicsComponent::CollisionShape::CAPSULE)
			{
				collisionShapeComboBox->SetSelected(2);
			}
			else if (physicsComponent->shape == RigidBodyPhysicsComponent::CollisionShape::CONVEX_HULL)
			{
				collisionShapeComboBox->SetSelected(3);
			}
			else if (physicsComponent->shape == RigidBodyPhysicsComponent::CollisionShape::TRIANGLE_MESH)
			{
				collisionShapeComboBox->SetSelected(4);
			}
		}

	}

	objectWindow->SetEnabled(!editor->selected.empty());

}
