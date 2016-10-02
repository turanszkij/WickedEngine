#include "stdafx.h"
#include "MeshWindow.h"


MeshWindow::MeshWindow(wiGUI* gui) : GUI(gui)
{
	assert(GUI && "Invalid GUI!");

	float screenW = (float)wiRenderer::GetDevice()->GetScreenWidth();
	float screenH = (float)wiRenderer::GetDevice()->GetScreenHeight();


	meshWindow = new wiWindow(GUI, "Mesh Window");
	meshWindow->SetSize(XMFLOAT2(400, 300));
	meshWindow->SetEnabled(false);
	GUI->AddWidget(meshWindow);

	float x = 200;
	float y = 0;

	doubleSidedCheckBox = new wiCheckBox("Double Sided: ");
	doubleSidedCheckBox->SetPos(XMFLOAT2(x, y += 30));
	doubleSidedCheckBox->OnClick([&](wiEventArgs args) {
		if (mesh != nullptr)
		{
			mesh->doubleSided = args.bValue;
		}
	});
	meshWindow->AddWidget(doubleSidedCheckBox);

	massSlider = new wiSlider(0, 5000, 0, 100000, "Mass: ");
	massSlider->SetSize(XMFLOAT2(100, 30));
	massSlider->SetPos(XMFLOAT2(x, y += 30));
	massSlider->OnSlide([&](wiEventArgs args) {
		if (mesh != nullptr)
		{
			mesh->mass = args.fValue;
		}
	});
	meshWindow->AddWidget(massSlider);

	frictionSlider = new wiSlider(0, 5000, 0, 100000, "Friction: ");
	frictionSlider->SetSize(XMFLOAT2(100, 30));
	frictionSlider->SetPos(XMFLOAT2(x, y += 30));
	frictionSlider->OnSlide([&](wiEventArgs args) {
		if (mesh != nullptr)
		{
			mesh->friction = args.fValue;
		}
	});
	meshWindow->AddWidget(frictionSlider);

	impostorCreateButton = new wiButton("Create Impostor");
	impostorCreateButton->SetSize(XMFLOAT2(240, 30));
	impostorCreateButton->SetPos(XMFLOAT2(x - 50, y += 30));
	impostorCreateButton->OnClick([&](wiEventArgs args) {
		if (mesh != nullptr)
		{
			wiRenderer::CreateImpostor(mesh);
		}
	});
	meshWindow->AddWidget(impostorCreateButton);

	impostorDistanceSlider = new wiSlider(0, 1000, 100, 10000, "Impostor Distance: ");
	impostorDistanceSlider->SetSize(XMFLOAT2(100, 30));
	impostorDistanceSlider->SetPos(XMFLOAT2(x, y += 30));
	impostorDistanceSlider->OnSlide([&](wiEventArgs args) {
		if (mesh != nullptr)
		{
			mesh->impostorDistance = args.fValue;
		}
	});
	meshWindow->AddWidget(impostorDistanceSlider);

	tessellationFactorSlider = new wiSlider(0, 16, 0, 10000, "Tessellation Factor: ");
	tessellationFactorSlider->SetSize(XMFLOAT2(100, 30));
	tessellationFactorSlider->SetPos(XMFLOAT2(x, y += 30));
	tessellationFactorSlider->OnSlide([&](wiEventArgs args) {
		if (mesh != nullptr)
		{
			mesh->tessellationFactor = args.fValue;
		}
	});
	meshWindow->AddWidget(tessellationFactorSlider);




	meshWindow->Translate(XMFLOAT3(30, 30, 0));
	meshWindow->SetVisible(false);
}


MeshWindow::~MeshWindow()
{
	SAFE_DELETE(meshWindow);
	SAFE_DELETE(doubleSidedCheckBox);
	SAFE_DELETE(massSlider);
	SAFE_DELETE(frictionSlider);
	SAFE_DELETE(impostorCreateButton);
	SAFE_DELETE(impostorDistanceSlider);
	SAFE_DELETE(tessellationFactorSlider);
}

void MeshWindow::SetMesh(Mesh* mesh)
{
	this->mesh = mesh;
	if (mesh != nullptr)
	{
		doubleSidedCheckBox->SetCheck(mesh->doubleSided);
		massSlider->SetValue(mesh->mass);
		frictionSlider->SetValue(mesh->friction);
		impostorDistanceSlider->SetValue(mesh->impostorDistance);
		tessellationFactorSlider->SetValue(mesh->getTessellationFactor());
		meshWindow->SetEnabled(true);
	}
	else
	{
		meshWindow->SetEnabled(false);
	}
}
