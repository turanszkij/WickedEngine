#include "stdafx.h"
#include "MeshWindow.h"

#include <sstream>

using namespace std;

MeshWindow::MeshWindow(wiGUI* gui) : GUI(gui)
{
	assert(GUI && "Invalid GUI!");

	float screenW = (float)wiRenderer::GetDevice()->GetScreenWidth();
	float screenH = (float)wiRenderer::GetDevice()->GetScreenHeight();


	meshWindow = new wiWindow(GUI, "Mesh Window");
	meshWindow->SetSize(XMFLOAT2(800, 600));
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
		if (mesh != nullptr)
		{
			mesh->doubleSided = args.bValue;
		}
	});
	meshWindow->AddWidget(doubleSidedCheckBox);

	massSlider = new wiSlider(0, 5000, 0, 100000, "Mass: ");
	massSlider->SetTooltip("Set the mass amount for the physics engine.");
	massSlider->SetSize(XMFLOAT2(100, 30));
	massSlider->SetPos(XMFLOAT2(x, y += step));
	massSlider->OnSlide([&](wiEventArgs args) {
		if (mesh != nullptr)
		{
			mesh->mass = args.fValue;
		}
	});
	meshWindow->AddWidget(massSlider);

	frictionSlider = new wiSlider(0, 5000, 0, 100000, "Friction: ");
	frictionSlider->SetTooltip("Set the friction amount for the physics engine.");
	frictionSlider->SetSize(XMFLOAT2(100, 30));
	frictionSlider->SetPos(XMFLOAT2(x, y += step));
	frictionSlider->OnSlide([&](wiEventArgs args) {
		if (mesh != nullptr)
		{
			mesh->friction = args.fValue;
		}
	});
	meshWindow->AddWidget(frictionSlider);

	impostorCreateButton = new wiButton("Create Impostor");
	impostorCreateButton->SetTooltip("Create an impostor image of the mesh. The mesh will be replaced by this image when far away, to render faster.");
	impostorCreateButton->SetSize(XMFLOAT2(240, 30));
	impostorCreateButton->SetPos(XMFLOAT2(x - 50, y += step));
	impostorCreateButton->OnClick([&](wiEventArgs args) {
		if (mesh != nullptr)
		{
			wiRenderer::CreateImpostor(mesh);
		}
	});
	meshWindow->AddWidget(impostorCreateButton);

	impostorDistanceSlider = new wiSlider(0, 1000, 100, 10000, "Impostor Distance: ");
	impostorDistanceSlider->SetTooltip("Assign the distance where the mesh geometry should be switched to the impostor image.");
	impostorDistanceSlider->SetSize(XMFLOAT2(100, 30));
	impostorDistanceSlider->SetPos(XMFLOAT2(x, y += step));
	impostorDistanceSlider->OnSlide([&](wiEventArgs args) {
		if (mesh != nullptr)
		{
			mesh->impostorDistance = args.fValue;
		}
	});
	meshWindow->AddWidget(impostorDistanceSlider);

	tessellationFactorSlider = new wiSlider(0, 16, 0, 10000, "Tessellation Factor: ");
	tessellationFactorSlider->SetTooltip("Set the dynamic tessellation amount. Tessellation should be enabled in the Renderer window and your GPU must support it!");
	tessellationFactorSlider->SetSize(XMFLOAT2(100, 30));
	tessellationFactorSlider->SetPos(XMFLOAT2(x, y += step));
	tessellationFactorSlider->OnSlide([&](wiEventArgs args) {
		if (mesh != nullptr)
		{
			mesh->tessellationFactor = args.fValue;
		}
	});
	meshWindow->AddWidget(tessellationFactorSlider);

	computeNormalsSmoothButton = new wiButton("Compute Normals [SMOOTH]");
	computeNormalsSmoothButton->SetTooltip("Compute surface normals of the mesh. Resulting normals will be unique per vertex.");
	computeNormalsSmoothButton->SetSize(XMFLOAT2(240, 30));
	computeNormalsSmoothButton->SetPos(XMFLOAT2(x - 50, y += step));
	computeNormalsSmoothButton->OnClick([&](wiEventArgs args) {
		if (mesh != nullptr)
		{
			mesh->ComputeNormals(true);
			SetMesh(mesh);
		}
	});
	meshWindow->AddWidget(computeNormalsSmoothButton);

	computeNormalsHardButton = new wiButton("Compute Normals [HARD]");
	computeNormalsHardButton->SetTooltip("Compute surface normals of the mesh. Resulting normals will be unique per face.");
	computeNormalsHardButton->SetSize(XMFLOAT2(240, 30));
	computeNormalsHardButton->SetPos(XMFLOAT2(x - 50, y += step));
	computeNormalsHardButton->OnClick([&](wiEventArgs args) {
		if (mesh != nullptr)
		{
			mesh->ComputeNormals(false);
			SetMesh(mesh);
		}
	});
	meshWindow->AddWidget(computeNormalsHardButton);




	meshWindow->Translate(XMFLOAT3(1300, 520, 0));
	meshWindow->SetVisible(false);

	SetMesh(nullptr);
}


MeshWindow::~MeshWindow()
{
	meshWindow->RemoveWidgets(true);
	GUI->RemoveWidget(meshWindow);
	SAFE_DELETE(meshWindow);
}

void MeshWindow::SetMesh(Mesh* mesh)
{
	this->mesh = mesh;
	if (mesh != nullptr)
	{
		stringstream ss("");
		ss << "Mesh name: " << mesh->name << endl;
		ss << "Vertex count: " << mesh->vertices_POS.size() << endl;
		ss << "Index count: " << mesh->indices.size() << endl;
		ss << "Subset count: " << mesh->subsets.size() << endl;
		meshInfoLabel->SetText(ss.str());

		doubleSidedCheckBox->SetCheck(mesh->doubleSided);
		massSlider->SetValue(mesh->mass);
		frictionSlider->SetValue(mesh->friction);
		impostorDistanceSlider->SetValue(mesh->impostorDistance);
		tessellationFactorSlider->SetValue(mesh->getTessellationFactor());
		meshWindow->SetEnabled(true);
	}
	else
	{
		meshInfoLabel->SetText("Select a mesh...");
		meshWindow->SetEnabled(false);
	}
}
