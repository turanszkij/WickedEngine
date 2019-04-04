#include "stdafx.h"
#include "MeshWindow.h"

#include <sstream>

using namespace std;
using namespace wiECS;
using namespace wiSceneSystem;

MeshWindow::MeshWindow(wiGUI* gui) : GUI(gui)
{
	assert(GUI && "Invalid GUI!");

	float screenW = (float)wiRenderer::GetDevice()->GetScreenWidth();
	float screenH = (float)wiRenderer::GetDevice()->GetScreenHeight();


	meshWindow = new wiWindow(GUI, "Mesh Window");
	meshWindow->SetSize(XMFLOAT2(800, 700));
	meshWindow->SetEnabled(false);
	GUI->AddWidget(meshWindow);

	float x = 200;
	float y = 0;
	float step = 35;

	meshInfoLabel = new wiLabel("Mesh Info");
	meshInfoLabel->SetPos(XMFLOAT2(x, y += step));
	meshInfoLabel->SetSize(XMFLOAT2(400, 150));
	meshWindow->AddWidget(meshInfoLabel);

	y += 160;

	doubleSidedCheckBox = new wiCheckBox("Double Sided: ");
	doubleSidedCheckBox->SetTooltip("If enabled, the inside of the mesh will be visible.");
	doubleSidedCheckBox->SetPos(XMFLOAT2(x, y += step));
	doubleSidedCheckBox->OnClick([&](wiEventArgs args) {
		MeshComponent* mesh = wiSceneSystem::GetScene().meshes.GetComponent(entity);
		if (mesh != nullptr)
		{
			mesh->SetDoubleSided(args.bValue);
		}
	});
	meshWindow->AddWidget(doubleSidedCheckBox);

	softbodyCheckBox = new wiCheckBox("Soft body: ");
	softbodyCheckBox->SetTooltip("Enable soft body simulation.");
	softbodyCheckBox->SetPos(XMFLOAT2(x, y += step));
	softbodyCheckBox->OnClick([&](wiEventArgs args) {

		Scene& scene = wiSceneSystem::GetScene();
		SoftBodyPhysicsComponent* physicscomponent = scene.softbodies.GetComponent(entity);

		if (args.bValue)
		{
			if (physicscomponent == nullptr)
			{
				SoftBodyPhysicsComponent& softbody = scene.softbodies.Create(entity);
				softbody.friction = frictionSlider->GetValue();
				softbody.mass = massSlider->GetValue();
			}
		}
		else
		{
			if (physicscomponent != nullptr)
			{
				scene.softbodies.Remove(entity);
			}
		}

	});
	meshWindow->AddWidget(softbodyCheckBox);

	massSlider = new wiSlider(0, 10, 0, 100000, "Mass: ");
	massSlider->SetTooltip("Set the mass amount for the physics engine.");
	massSlider->SetSize(XMFLOAT2(100, 30));
	massSlider->SetPos(XMFLOAT2(x, y += step));
	massSlider->OnSlide([&](wiEventArgs args) {
		SoftBodyPhysicsComponent* physicscomponent = wiSceneSystem::GetScene().softbodies.GetComponent(entity);
		if (physicscomponent != nullptr)
		{
			physicscomponent->mass = args.fValue;
		}
	});
	meshWindow->AddWidget(massSlider);

	frictionSlider = new wiSlider(0, 2, 0, 100000, "Friction: ");
	frictionSlider->SetTooltip("Set the friction amount for the physics engine.");
	frictionSlider->SetSize(XMFLOAT2(100, 30));
	frictionSlider->SetPos(XMFLOAT2(x, y += step));
	frictionSlider->OnSlide([&](wiEventArgs args) {
		SoftBodyPhysicsComponent* physicscomponent = wiSceneSystem::GetScene().softbodies.GetComponent(entity);
		if (physicscomponent != nullptr)
		{
			physicscomponent->friction = args.fValue;
		}
	});
	meshWindow->AddWidget(frictionSlider);

	impostorCreateButton = new wiButton("Create Impostor");
	impostorCreateButton->SetTooltip("Create an impostor image of the mesh. The mesh will be replaced by this image when far away, to render faster.");
	impostorCreateButton->SetSize(XMFLOAT2(240, 30));
	impostorCreateButton->SetPos(XMFLOAT2(x - 50, y += step));
	impostorCreateButton->OnClick([&](wiEventArgs args) {
		MeshComponent* mesh = wiSceneSystem::GetScene().meshes.GetComponent(entity);
		if (mesh != nullptr)
		{
			Scene& scene = wiSceneSystem::GetScene();
			scene.impostors.Create(entity).swapInDistance = impostorDistanceSlider->GetValue();
		}
	});
	meshWindow->AddWidget(impostorCreateButton);

	impostorDistanceSlider = new wiSlider(0, 1000, 100, 10000, "Impostor Distance: ");
	impostorDistanceSlider->SetTooltip("Assign the distance where the mesh geometry should be switched to the impostor image.");
	impostorDistanceSlider->SetSize(XMFLOAT2(100, 30));
	impostorDistanceSlider->SetPos(XMFLOAT2(x, y += step));
	impostorDistanceSlider->OnSlide([&](wiEventArgs args) {
		ImpostorComponent* impostor = wiSceneSystem::GetScene().impostors.GetComponent(entity);
		if (impostor != nullptr)
		{
			impostor->swapInDistance = args.fValue;
		}
	});
	meshWindow->AddWidget(impostorDistanceSlider);

	tessellationFactorSlider = new wiSlider(0, 16, 0, 10000, "Tessellation Factor: ");
	tessellationFactorSlider->SetTooltip("Set the dynamic tessellation amount. Tessellation should be enabled in the Renderer window and your GPU must support it!");
	tessellationFactorSlider->SetSize(XMFLOAT2(100, 30));
	tessellationFactorSlider->SetPos(XMFLOAT2(x, y += step));
	tessellationFactorSlider->OnSlide([&](wiEventArgs args) {
		MeshComponent* mesh = wiSceneSystem::GetScene().meshes.GetComponent(entity);
		if (mesh != nullptr)
		{
			mesh->tessellationFactor = args.fValue;
		}
	});
	meshWindow->AddWidget(tessellationFactorSlider);

	flipCullingButton = new wiButton("Flip Culling");
	flipCullingButton->SetTooltip("Flip faces to reverse triangle culling order.");
	flipCullingButton->SetSize(XMFLOAT2(240, 30));
	flipCullingButton->SetPos(XMFLOAT2(x - 50, y += step));
	flipCullingButton->OnClick([&](wiEventArgs args) {
		MeshComponent* mesh = wiSceneSystem::GetScene().meshes.GetComponent(entity);
		if (mesh != nullptr)
		{
			mesh->FlipCulling();
			SetEntity(entity);
		}
	});
	meshWindow->AddWidget(flipCullingButton);

	flipNormalsButton = new wiButton("Flip Normals");
	flipNormalsButton->SetTooltip("Flip surface normals.");
	flipNormalsButton->SetSize(XMFLOAT2(240, 30));
	flipNormalsButton->SetPos(XMFLOAT2(x - 50, y += step));
	flipNormalsButton->OnClick([&](wiEventArgs args) {
		MeshComponent* mesh = wiSceneSystem::GetScene().meshes.GetComponent(entity);
		if (mesh != nullptr)
		{
			mesh->FlipNormals();
			SetEntity(entity);
		}
	});
	meshWindow->AddWidget(flipNormalsButton);

	computeNormalsSmoothButton = new wiButton("Compute Normals [SMOOTH]");
	computeNormalsSmoothButton->SetTooltip("Compute surface normals of the mesh. Resulting normals will be unique per vertex.");
	computeNormalsSmoothButton->SetSize(XMFLOAT2(240, 30));
	computeNormalsSmoothButton->SetPos(XMFLOAT2(x - 50, y += step));
	computeNormalsSmoothButton->OnClick([&](wiEventArgs args) {
		MeshComponent* mesh = wiSceneSystem::GetScene().meshes.GetComponent(entity);
		if (mesh != nullptr)
		{
			mesh->ComputeNormals(true);
			SetEntity(entity);
		}
	});
	meshWindow->AddWidget(computeNormalsSmoothButton);

	computeNormalsHardButton = new wiButton("Compute Normals [HARD]");
	computeNormalsHardButton->SetTooltip("Compute surface normals of the mesh. Resulting normals will be unique per face.");
	computeNormalsHardButton->SetSize(XMFLOAT2(240, 30));
	computeNormalsHardButton->SetPos(XMFLOAT2(x - 50, y += step));
	computeNormalsHardButton->OnClick([&](wiEventArgs args) {
		MeshComponent* mesh = wiSceneSystem::GetScene().meshes.GetComponent(entity);
		if (mesh != nullptr)
		{
			mesh->ComputeNormals(false);
			SetEntity(entity);
		}
	});
	meshWindow->AddWidget(computeNormalsHardButton);

	recenterButton = new wiButton("Recenter");
	recenterButton->SetTooltip("Recenter mesh to AABB center.");
	recenterButton->SetSize(XMFLOAT2(240, 30));
	recenterButton->SetPos(XMFLOAT2(x - 50, y += step));
	recenterButton->OnClick([&](wiEventArgs args) {
		MeshComponent* mesh = wiSceneSystem::GetScene().meshes.GetComponent(entity);
		if (mesh != nullptr)
		{
			mesh->Recenter();
			SetEntity(entity);
		}
	});
	meshWindow->AddWidget(recenterButton);

	recenterToBottomButton = new wiButton("RecenterToBottom");
	recenterToBottomButton->SetTooltip("Recenter mesh to AABB bottom.");
	recenterToBottomButton->SetSize(XMFLOAT2(240, 30));
	recenterToBottomButton->SetPos(XMFLOAT2(x - 50, y += step));
	recenterToBottomButton->OnClick([&](wiEventArgs args) {
		MeshComponent* mesh = wiSceneSystem::GetScene().meshes.GetComponent(entity);
		if (mesh != nullptr)
		{
			mesh->RecenterToBottom();
			SetEntity(entity);
		}
	});
	meshWindow->AddWidget(recenterToBottomButton);




	meshWindow->Translate(XMFLOAT3(screenW - 910, 520, 0));
	meshWindow->SetVisible(false);

	SetEntity(INVALID_ENTITY);
}


MeshWindow::~MeshWindow()
{
	meshWindow->RemoveWidgets(true);
	GUI->RemoveWidget(meshWindow);
	SAFE_DELETE(meshWindow);
}

void MeshWindow::SetEntity(Entity entity)
{
	this->entity = entity;

	Scene&scene = wiSceneSystem::GetScene();

	const MeshComponent* mesh = scene.meshes.GetComponent(entity);

	if (mesh != nullptr)
	{
		const NameComponent& name = *scene.names.GetComponent(entity);

		stringstream ss("");
		ss << "Mesh name: " << name.name << endl;
		ss << "Vertex count: " << mesh->vertex_positions.size() << endl;
		ss << "Index count: " << mesh->indices.size() << endl;
		ss << "Subset count: " << mesh->subsets.size() << endl;
		ss << endl << "Vertex buffers: ";
		if (mesh->vertexBuffer_POS != nullptr) ss << "position; ";
		if (mesh->vertexBuffer_UV0 != nullptr) ss << "uvset_0; ";
		if (mesh->vertexBuffer_UV1 != nullptr) ss << "uvset_1; ";
		if (mesh->vertexBuffer_ATL != nullptr) ss << "atlas; ";
		if (mesh->vertexBuffer_COL != nullptr) ss << "color; ";
		if (mesh->vertexBuffer_PRE != nullptr) ss << "prevPos; ";
		if (mesh->vertexBuffer_BON != nullptr) ss << "bone; ";
		if (mesh->streamoutBuffer_POS != nullptr) ss << "streamout; ";
		meshInfoLabel->SetText(ss.str());

		doubleSidedCheckBox->SetCheck(mesh->IsDoubleSided());

		const ImpostorComponent* impostor = scene.impostors.GetComponent(entity);
		if (impostor != nullptr)
		{
			impostorDistanceSlider->SetValue(impostor->swapInDistance);
		}
		tessellationFactorSlider->SetValue(mesh->GetTessellationFactor());

		softbodyCheckBox->SetCheck(false);

		SoftBodyPhysicsComponent* physicscomponent = wiSceneSystem::GetScene().softbodies.GetComponent(entity);
		if (physicscomponent != nullptr)
		{
			softbodyCheckBox->SetCheck(true);
			massSlider->SetValue(physicscomponent->mass);
			frictionSlider->SetValue(physicscomponent->friction);
		}
		meshWindow->SetEnabled(true);
	}
	else
	{
		meshInfoLabel->SetText("Select a mesh...");
		meshWindow->SetEnabled(false);
	}
}
